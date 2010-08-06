// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\defrag\t_ramdefrag.cpp
// RAM Defragmentation Functional Tests
// 
//

//#define RUN_ALL_TESTS			// Uncomment to ensure that all tests are run regardless of test failures
//#define DEBUG_VER				// Uncomment for information output from tests



#define __E32TEST_EXTENSION__
#include <e32test.h>
RTest test(_L("T_RAMDEFRAG"));
#include <e32rom.h>
#include <u32hal.h>
#include <f32file.h>
#include <f32dbg.h>
#include <e32svr.h>
#include <e32msgqueue.h>
#include <e32math.h>
#include <hal.h>
#include "testdefs.h"
#include "..\mmu\mmudetect.h"


#include <dptest.h>

#include "t_ramdefrag.h"

#define READ(a) ReadByte((volatile TUint8*)(a))


#ifdef RUN_ALL_TESTS
#define TEST_FAIL {gTestStepFailed++;}
#define CLEANUP(a) {}
#else
#define TEST_FAIL {TestCleanup(); test(EFalse);}
#define CLEANUP(a) {if (!gFailPrintPageInfo) \
						{ \
						PrintPageInfo(); \
						gFailPrintPageInfo = ETrue; \
						} \
					a;}
#endif



#define TEST_DRIVER_OPEN		1
#define TEST_DRIVER_CLOSE		0

#define BEST_MOVABLE			1
#define BEST_DISCARDABLE		2
#define BEST_FIXED				3

#define Z_ALLOC_CONTIG			1
#define Z_ALLOC_DISC			2

LOCAL_D TUint gTestStarted = EFalse;								// Used to ensure matching TestStart() and TestEnd().
LOCAL_D TBool gPagedRom = ETrue;									// Stores whether or not is a paged ROM
LOCAL_D TInt gTestStepFailed = 0;									// Stores the number of test steps failed
LOCAL_D TBool gFailPrintPageInfo = EFalse;							// Set to ETrue once CLEANUP has been invoked once.
LOCAL_D TBool gFileCacheRun = EFalse;								// Set to ETrue whe FSCaching tests have been run

LOCAL_D TInt gRamSize;												// The total RAM size in bytes
LOCAL_D TInt gFreeRam;												// The amount of free RAM available in bytes
LOCAL_D TInt gPageSize;												// The number of bytes per page
LOCAL_D TUint gPageShift;
#ifdef DEBUG_VER
LOCAL_D TInt gRamUsed; 												// The amount of RAM used in bytes
#endif
LOCAL_D TInt gInitialRam;											// The initial free RAM before a test starts
LOCAL_D TInt gEndRam;												// The end free RAM when a test finishes
LOCAL_D TUint gOriginalMinCacheSize;									// The original DP minSize
LOCAL_D TUint gOriginalMaxCacheSize;									// The original DP maxSize
LOCAL_D TInt gTotalRamLost;											// The total amount of RAM lost during the test

LOCAL_D TUint gZoneCount = 0;										// Number of zones
LOCAL_D const TUint KInvalidZoneID = 0xffffffff;					// Invalid value for a zone ID
LOCAL_D STestPageCount gTotalPageCount;


LOCAL_D struct SRamZoneConfig*	gZoneConfigArray;					// Contains the configurations of all the zones
LOCAL_D struct SRamZoneUtilisation*	gZoneUtilArray;					// Contains the utilisations of all the zones
LOCAL_D struct SRamZoneUtilisation* gOriginalPageCountArray;		// Contains the original utilisations of the zones
LOCAL_D TInt* gPrefArray;											// Contains the preference order of the zones 
LOCAL_D TUint8* gOrigFlagArray;										// Contains the orignal values for the zone flags

LOCAL_D TInt gDefragMaxPages = 0;

const TInt KFillAllMovable = -1;

LOCAL_D RChunk* gChunkArray1 = NULL;								// Stores reference to all the chunks that have been created
LOCAL_D RChunk* gChunkArray2 = NULL;								// Stores reference to all the chunks that have been created
LOCAL_D TUint gChunkArraySize1 = 0;									// The size of the array gChunkArray
LOCAL_D TUint gChunkArraySize2 = 0;									// The size of the array gChunkArray
const TUint KChunkDefaultSize = 0x300000;	
const TUint KMaxChunks = 14;
LOCAL_D const TUint KNumAllocChunks = 10;							// The number of chunks to be allocd for some tests.
	
LOCAL_D RRamDefragFuncTestLdd Ldd;									// Main Ldd used to call into device driver

LOCAL_D TBuf<20> gTestThreadName =_L("TestThread");	
LOCAL_D RThread gTestThread;
LOCAL_D TRequestStatus status;

LOCAL_D TInt gDrive;												// The removable media drive
LOCAL_D RFs gTheFs;									
LOCAL_D TFileName gSessionPath;

const TInt KNoRemovableDrive = -1;									// gDrive is set to this when no suitable drive can be found.
const TInt KDefaultCacheSize = (128 + 12) * 1024;					// The default file system cache size 
const TUint KNumFilesOrig = (32 * 1024 * 1024) / KDefaultCacheSize;	// The number of files that are needed to fill the file system cache
LOCAL_D TInt gFilesNeededToFillCache = KNumFilesOrig ;				// Not constant as can change depending on the size of the disk
LOCAL_D RFile gFile[KNumFilesOrig];

LOCAL_D TInt* gCandList1;											// Array of zones that have the same preference and the same
LOCAL_D TInt* gCandList2;											// amount of free pages
const TInt KInvalidCandIndex = -1;
LOCAL_D TUint gMemModel;

//
// GetDrive
//
// Gets the removable drive number
//
TInt GetDrive()
	{
	RFs theFs;
	TInt r = theFs.Connect();
	test_KErrNone(r);

	TInt driveLet = KNoRemovableDrive;

	TInt i = EDriveA;
	for (; i <= EDriveZ; i++)
		{
		TVolumeInfo volInfo;
		r = theFs.Volume(volInfo, i);
		if (r == KErrNone)
			{// This drive no. exists so determine if it is removable and 
			//  formattable media.
			if ((volInfo.iDrive.iDriveAtt & KDriveAttRemovable) &&
				(volInfo.iDrive.iMediaAtt & KMediaAttFormattable))	
				{
				driveLet = i;
				break;
				}
			}
		}
	theFs.Close();
	return driveLet;
	}


//
// DeviceDriver
//
// Opens or closes the device driver used
//
TInt DeviceDriver(TInt aFunctionNum)
	{
	TInt r = 0;
	switch (aFunctionNum)
		{
		case TEST_DRIVER_OPEN:
			{
			r = User::LoadLogicalDevice(KRamDefragFuncTestLddName);
			test(r==KErrNone || r==KErrAlreadyExists);
			r = Ldd.Open();
			test_KErrNone(r);
			}
		break;
		
		case TEST_DRIVER_CLOSE:
			{
			Ldd.Close();
			r = User::FreeLogicalDevice(KRamDefragFuncTestLddName);
			test_KErrNone(r);
			}
		break;
		
		default:
		break;
		
		}
	return r;
	}


//
// GetOriginalPageCount
//
// Obtains the orginal types of pages in each of the zones
//
void GetOriginalPageCount()
	{
	TUint index;
	TInt ret = 0;
	TESTDEBUG(test.Printf(_L("ram defrag : Get info about the zones\n")));
	for (index = 0; index < gZoneCount; index ++)
		{
		ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation,(TAny*)index, (TAny*)&gOriginalPageCountArray[index]);
		test(ret == KErrNone);
		}
	}


//
// PrintPageInfo
//
// Prints various page information to the screen
//
void PrintPageInfo()
	{
	test.Printf(_L("\nZONE CONFIGURATIONS:\n"));
	for (TUint index = 0; index < gZoneCount; index ++)
		{
		TInt ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneConfig,(TAny*)index, (TAny*)&gZoneConfigArray[index]);
		test(ret == KErrNone);
		test.Printf(_L("config : id=0x%08x  index=%-2d  base=0x%08x  end=0x%08x  pages=0x%08x  pref=%-2d  flags=0x%x\n"),
					gZoneConfigArray[index].iZoneId,gZoneConfigArray[index].iZoneIndex,
					gZoneConfigArray[index].iPhysBase,gZoneConfigArray[index].iPhysEnd,
					gZoneConfigArray[index].iPhysPages,gZoneConfigArray[index].iPref,gZoneConfigArray[index].iFlags);
		}
	test.Printf(_L("\nZONE UTILISATIONS:\n"));
	for (TUint index = 0; index < gZoneCount; index ++)
		{
		TInt ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation,(TAny*)index, (TAny*)&gZoneUtilArray[index]);
		test(ret == KErrNone);
		test.Printf(_L("usage  : id=0x%08x  index=%-2d  pref=%d  pages=0x%08x free=0x%08x  unknown=0x%08x  fixed=0x%08x  move=0x%08x  discard=0x%08x  other=0x%08x\n"),
					gZoneUtilArray[index].iZoneId,gZoneUtilArray[index].iZoneIndex,gZoneConfigArray[index].iPref, 
					gZoneUtilArray[index].iPhysPages,gZoneUtilArray[index].iFreePages,
					gZoneUtilArray[index].iAllocUnknown,gZoneUtilArray[index].iAllocFixed,gZoneUtilArray[index].iAllocMovable,
					gZoneUtilArray[index].iAllocDiscardable,gZoneUtilArray[index].iAllocOther);
		}
	}


//
// GetAllPageInfo
//
// Get various different page information for all zones
// Also updates the total page count
//
void GetAllPageInfo()
	{
	TInt ret = 0;
	gTotalPageCount.iFreePages = 0;
	gTotalPageCount.iUnknownPages = 0;
	gTotalPageCount.iFixedPages = 0;
	gTotalPageCount.iMovablePages = 0;
	gTotalPageCount.iDiscardablePages = 0;
	gTotalPageCount.iOtherPages = 0;

	// now get the config of each of the zones.
	TUint	index;
	TESTDEBUG(test.Printf(_L("ram defrag : Get info about the zones\n")));
	for (index = 0; index < gZoneCount; index ++)
		{
		ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneConfig,(TAny*)index, (TAny*)&gZoneConfigArray[index]);
		test(ret == KErrNone);

		ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneUtilisation,(TAny*)index, (TAny*)&gZoneUtilArray[index]);
		test(ret == KErrNone);
		
		gTotalPageCount.iFreePages += gZoneUtilArray[index].iFreePages;
		gTotalPageCount.iUnknownPages += gZoneUtilArray[index].iAllocUnknown;
		gTotalPageCount.iFixedPages += gZoneUtilArray[index].iAllocFixed;
		gTotalPageCount.iMovablePages += gZoneUtilArray[index].iAllocMovable;
		gTotalPageCount.iDiscardablePages += gZoneUtilArray[index].iAllocDiscardable;
		gTotalPageCount.iOtherPages += gZoneUtilArray[index].iAllocOther;
		}
	TESTDEBUG(test.Printf(_L("free=0x%x unknown=0x%x fixed=0x%x move=0x%x discard=0x%x other=0x%x\n"),
					gTotalPageCount.iFreePages, gTotalPageCount.iUnknownPages, gTotalPageCount.iFixedPages, 
					gTotalPageCount.iMovablePages, gTotalPageCount.iDiscardablePages,gTotalPageCount.iOtherPages));

	TESTDEBUG(PrintPageInfo());
	}

void RestoreRamZoneFlags()
	{
	GetAllPageInfo(); // Update the current set of RAM zone flag data.
	for (TUint index=0; index < gZoneCount; index++)
		{
		TUint zoneDefragID = gZoneConfigArray[index].iZoneId;
		Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, ORIG_FLAG, gOrigFlagArray[index]);
		}
	}

void ResetRamZoneFlags()
	{
	GetAllPageInfo(); // Update the current set of RAM zone flag data.
	for (TUint index=0; index < gZoneCount; index++)
		{
		TUint zoneDefragID = gZoneConfigArray[index].iZoneId;
		Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, RESET_FLAG);
		}
	}

void RemoveChunkAlloc(RChunk*& aChunkArray, TUint& aChunkArraySize);
void ResetDPCache();

void FSCachCleanUp()
	{
	// If the File System Caching tests have been run, 
	// ensure that they are cleaned up
	if (gFileCacheRun)
		{
		TUint i = 0;
		// First close all the open handles to the RFile objects open
		for (i = 0; i < KNumFilesOrig; i++) 
			{
			gFile[i].Close();
			}

		// Now call EmptyRamZone on every zone to ensure that 
		// discardable pages are cleaned up
		
		GetAllPageInfo();
		for (i = 0; i < gZoneCount; i++)
			{
			TUint zoneID = gZoneConfigArray[i].iZoneId;
			Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, zoneID);
			}
		gFileCacheRun = EFalse;
		}
	}

//
// TestCleanup
//
// Cleans up all the allocations made at the beginning of the test
//
void TestCleanup()
	{

	Ldd.ResetDriver();

	// Revert the cleared flags to their original values before the tests were carried out
	RestoreRamZoneFlags();

	// Reset the DP cache and remove any allocated chunks and fixed pages.
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	RemoveChunkAlloc(gChunkArray2, gChunkArraySize2);
	ResetDPCache();
	Ldd.FreeAllFixedPages();
	Ldd.FreeFromAllZones();
	FSCachCleanUp();

	User::Free(gPrefArray);
	gPrefArray = NULL;

	User::Free(gOrigFlagArray);
	gOrigFlagArray = NULL;

	User::Free(gCandList1);
	gCandList1 = NULL;

	User::Free(gCandList2);
	gCandList2 = NULL;

	User::Free(gOriginalPageCountArray);
	gOriginalPageCountArray = NULL;

	// Output the last possible state of memory
	if (!gFailPrintPageInfo)
		PrintPageInfo();

	User::Free(gZoneConfigArray);
	gZoneConfigArray = NULL;

	User::Free(gZoneUtilArray);
	gZoneUtilArray = NULL;

	}


// TestSetup
//
// Get the necessary information needed to carry out the tests
//
TInt TestSetup()
	{
	// Get the MMC drive
	gDrive = GetDrive();

	// first get the number of zones
	TInt ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneCount,&gZoneCount,0);
	if (ret != KErrNone)
		{
		test.Printf(_L("Cannot obtain the number of zones\n"));
		return ret;
		}
	test.Printf(_L("RAM Zones (count=%u)\n"),gZoneCount);

	// Obtain the size of the RAM and the size of a page	
	ret = HAL::Get(HAL::EMemoryRAM, gRamSize);
	if (ret != KErrNone)
		{
		test.Printf(_L("Cannot obtain the size of RAM\n"));
		return ret;
		}

	// Retrieve the page size and use it to detemine the page shift (assumes 32-bit system).
	ret = HAL::Get(HAL::EMemoryPageSize, gPageSize);
	if (ret != KErrNone)
		{
		test.Printf(_L("Cannot obtain the page size\n"));
		return ret;
		}

	TUint32 pageMask = gPageSize;
	TUint i = 0;
	for (; i < 32; i++)
		{
		if (pageMask & 1)
			{
			if (pageMask & ~1u)
				{
				test.Printf(_L("ERROR - page size not a power of 2"));
				return KErrNotSupported;
				}
			gPageShift = i;
			break;
			}
		pageMask >>= 1;
		}

	gZoneConfigArray = (SRamZoneConfig *)User::AllocZ(sizeof(SRamZoneConfig) * gZoneCount);
	if (gZoneConfigArray == NULL)
		return KErrNoMemory;
	gZoneUtilArray = (SRamZoneUtilisation *)User::AllocZ(sizeof(SRamZoneUtilisation) * gZoneCount);
	if (gZoneUtilArray == NULL)
		return KErrNoMemory;
	gPrefArray = (TInt *)User::AllocZ(sizeof(TInt) * gZoneCount);
	if (gPrefArray == NULL)
		return KErrNoMemory;
	gOrigFlagArray = (TUint8 *)User::AllocZ(sizeof(TUint8) * gZoneCount);
	if (gOrigFlagArray == NULL)
		return KErrNoMemory;
	gOriginalPageCountArray = (SRamZoneUtilisation *)User::AllocZ(sizeof(SRamZoneUtilisation) * gZoneCount);
	if (gOriginalPageCountArray == NULL)
		return KErrNoMemory;

	gCandList1 = (TInt *)User::AllocZ(sizeof(TInt) * gZoneCount);
	if (gCandList1 == NULL)
		return KErrNoMemory;
	gCandList2 = (TInt *)User::AllocZ(sizeof(TInt) * gZoneCount);
	if (gCandList2 == NULL)
		return KErrNoMemory;

	GetAllPageInfo();
	PrintPageInfo();
	
	// Store the original flags
	for (i=0; i < gZoneCount; i++)
		gOrigFlagArray[i] = gZoneConfigArray[i].iFlags;
	
	// Now clear the flags for carrying out tests
	Ldd.ResetDriver();
	ResetRamZoneFlags();
	
	
	// Check whether the ROM is paged or not
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	if(!romHeader->iPageableRomStart)
		{
		test.Printf(_L("Not a Paged ROM - Skipping all discardable page tests.\n"));
		gPagedRom = EFalse;
		}
	else
		{// Save the current state of the DP cache so it can be restored when required and
		// after the test has finished.
		TUint currentCacheSize;
		DPTest::CacheSize(gOriginalMinCacheSize, gOriginalMaxCacheSize, currentCacheSize);
		TESTDEBUG(test.Printf(_L("Original CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
									gOriginalMinCacheSize >> gPageShift, gOriginalMaxCacheSize >> gPageShift, 
									currentCacheSize >> gPageShift));
		}

	// Get the memory model of the kernel that this test is running on.
	gMemModel = MemModelType();
	return KErrNone;
	}


// 
// UpdateRamInfo
//
// Updating the various RAM information
//
void UpdateRamInfo()
	{
	HAL::Get(HAL::EMemoryRAMFree, gFreeRam);
	TESTDEBUG(gRamUsed = gRamSize - gFreeRam);
	}
	

// 
// CheckRamDifference
//
// Checks the difference between the initial free RAM and the end free RAM
//
void CheckRamDifference()
	{
	if (gInitialRam == gEndRam)
		{
		TESTDEBUG(test.Printf(_L("No RAM was lost during this test\n")));
		}
	else
		{
		TInt diff = gInitialRam - gEndRam;


		gTotalRamLost = gTotalRamLost + diff;
		}
	}

TInt VerifyMovDisAlloc();
//
// TestStart
//
// Updates the RAM information at the beginning of a test step	
//
void TestStart()
	{
	test(!gTestStarted);
	gTestStarted = ETrue;

	Ldd.ResetDriver();
	
	Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	TInt r;
	r = VerifyMovDisAlloc();
	if (r == KErrGeneral)
		{
		// A rare set of circumstances may cause some of the movable pages to be in
		// use during the defrag and thus not be moved to the correct RAM zone. Run
		// the defrag once more to give it a chance to move these pages. We ensure
		// that there is no pending asynchronous clean up operation before doing so.
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
		Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		r = VerifyMovDisAlloc();
		}

	if (r != KErrNone)
		{
		CLEANUP(;);
		TEST_FAIL;
		}
	
	UpdateRamInfo();
	gInitialRam = gFreeRam;
	TESTDEBUG(test.Printf(_L("Initial Free RAM = 0x%x, Initial RAM Used = 0x%x\n"), gFreeRam, gRamUsed));
	}


void RemoveChunkAlloc(RChunk*& aChunkArray, TUint& aChunkArraySize);
//
// TestEnd
//
// Updates RAM information at end of test step and checks the RAM delta
//
void TestEnd()
	{
	test(gTestStarted);
	gTestStarted = EFalse;

	gDefragMaxPages = 0;

	// Clean up anything that may need to be cleaned.
	ResetRamZoneFlags();
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	RemoveChunkAlloc(gChunkArray2, gChunkArraySize2);
	ResetDPCache();
	Ldd.FreeAllFixedPages();
	FSCachCleanUp();
	
	UpdateRamInfo();
	gEndRam = gFreeRam;
	TESTDEBUG(test.Printf(_L("End RAM Free = 0x%x, End RAM Used = 0x%x\n"), gEndRam, gRamUsed));

	// Ensure any asynchronous clean up operations complete before we move on 
	// to the next test.
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	CheckRamDifference();
	test.Printf(_L(" \n"));
	}

//
// CheckZonesSwitchedOff
//
// Checks that zones have been switched off
//
TBool CheckZonesSwitchedOff()
	{
	GetAllPageInfo();
	for (TUint i = 0; i < gZoneCount; i++)
		{
		if (gOriginalPageCountArray[i].iFreePages != gOriginalPageCountArray[i].iPhysPages &&
			gZoneUtilArray[i].iFreePages == gZoneUtilArray[i].iPhysPages)
			{
			return ETrue;
			}
		}
	return EFalse;
	}
	

//
// CheckZoneIsOff
//
// Checks if a particular zone is empty 
//
TBool CheckZoneIsOff(TUint aZoneIndex)
	{
	GetAllPageInfo();
	if (gZoneUtilArray[aZoneIndex].iFreePages == gZoneUtilArray[aZoneIndex].iPhysPages)
		{
		TESTDEBUG(test.Printf(_L("Zone index %d is Empty\n"), aZoneIndex));
		return ETrue;
		}
	else
		{
		TESTDEBUG(test.Printf(_L("Zone index %d is Not empty\n"), aZoneIndex));
		return EFalse;
		}
	}


//
// GetPrefOrder
//
// Go through each zone ordering them in preference order
//
void GetPrefOrder()
	{
	GetAllPageInfo();
	TESTDEBUG(PrintPageInfo());

	for (TUint i=0; i < (TUint)gZoneCount; i++)
		gPrefArray[i] = KErrNotFound;

	for (TUint curIndex = 0; curIndex < gZoneCount; curIndex++)
		{
		TBool currentEmpty = gZoneUtilArray[curIndex].iPhysPages == gZoneUtilArray[curIndex].iFreePages;
		TUint currentPref = gZoneConfigArray[curIndex].iPref;
		TUint currentImmovPages = 	gZoneUtilArray[curIndex].iAllocFixed + 
									gZoneUtilArray[curIndex].iAllocUnknown;
		TUint morePrefCnt = 0;
		for (TUint index = 0; index < gZoneCount; index++)
			{// A RAM zone with the same iPref is more preferable if it has 
			// more immovable pages.
			if (gZoneConfigArray[index].iPref < currentPref || 
				(gZoneConfigArray[index].iPref == currentPref && 
				(currentImmovPages < gZoneUtilArray[index].iAllocFixed + gZoneUtilArray[index].iAllocUnknown ||
				(currentEmpty &&
				gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages))))
				{
				morePrefCnt++;
				}
			}

		while (gPrefArray[morePrefCnt] != KErrNotFound)
			{// Zone(s) of this preference and size already exist so 
			 // place this one after it/them
			morePrefCnt++;
			}
		gPrefArray[morePrefCnt] = curIndex;
		}
	}

//
// ZonesSamePref
//
// Return ETrue if the RAM zones are of the same preference.
//
// NOTE - This requires GetAllPageInfo() to have already been called.
//
TBool ZonesSamePref(TUint aZoneIndex1, TUint aZoneIndex2)
	{
	TBool zoneEmpty1 = gZoneUtilArray[aZoneIndex1].iFreePages == gZoneUtilArray[aZoneIndex1].iPhysPages;
	TBool zoneEmpty2 = gZoneUtilArray[aZoneIndex2].iFreePages == gZoneUtilArray[aZoneIndex2].iPhysPages;
	if (gZoneConfigArray[aZoneIndex1].iPref == gZoneConfigArray[aZoneIndex2].iPref &&
		(gZoneUtilArray[aZoneIndex1].iAllocFixed + gZoneUtilArray[aZoneIndex1].iAllocUnknown == 
		gZoneUtilArray[aZoneIndex2].iAllocFixed + gZoneUtilArray[aZoneIndex2].iAllocUnknown &&
		(zoneEmpty1 == zoneEmpty2)))
		{
		return ETrue;
		}

	return EFalse;	
	}

//
// FindMostPrefEmpty
//
// Checks all zones and returns the most preferable RAM zone which 
// is completely emtpy
//
// @param aZoneIndex On return this will contain the index into gZoneUtilArray of the most preferable empty RAM zone.
// @param aPrefIndex On return this will contain the index into gPrefArray of the most preferable empty RAM zone.
//
// @return KErrNotFound if a zone cannot be found, else KErrNone
//
TInt FindMostPrefEmpty(TUint& aZoneIndex, TUint* aPrefIndex = NULL)
	{
	// Get the most pref zone which is completely free to use as a test zone
	GetPrefOrder();
	TUint prefIndex = 0;
	for (; prefIndex < gZoneCount; prefIndex++)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		if (gZoneUtilArray[zoneIndex].iFreePages == gZoneUtilArray[zoneIndex].iPhysPages)
			{
			aZoneIndex = zoneIndex;
			if (aPrefIndex)
				{
				*aPrefIndex = prefIndex;
				}
			return KErrNone;
			}
		}
	return KErrNotFound;
	}

//
// FindLeastPrefEmpty
//
// Checks all zones and returns the least preferable RAM zone which 
// is completely emtpy
//
// @param aZoneIndex On return this will contain the index into gZoneUtilArray of the least preferable empty RAM zone.
// @param aPrefIndex On return this will contain the index into gPrefArray of the least preferable empty RAM zone.
//
// @return KErrNotFound if a zone cannot be found, else KErrNone
//
TInt FindLeastPrefEmpty(TUint& aZoneIndex, TUint* aPrefIndex = NULL)
	{
	// Get the most pref zone which is completely free to use as a test zone
	GetPrefOrder();
	TInt prefIndex = gZoneCount - 1;
	for (; prefIndex >= 0; prefIndex--)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		if (gZoneUtilArray[zoneIndex].iFreePages == gZoneUtilArray[zoneIndex].iPhysPages)
			{
			aZoneIndex = zoneIndex;
			if (aPrefIndex)
				{
				*aPrefIndex = (TUint)prefIndex;
				}
			return KErrNone;
			}
		}
	return KErrNotFound;
	}

//
// FindMostPrefWithFree
//
// Checks all zones and returns the most preferable RAM zone which 
// has at least 1 free page 
//
// @param aZoneIndex On return this will contain the index into gZoneUtilArray of the most preferable RAM zone with free pages.
// @param aPrefIndex On return this will contain the index into gPrefArray of the most preferable RAM zone with free pages.
//
// @return KErrNotFound if a zone cannot be found, else KErrNone
//
TInt FindMostPrefWithFree(TUint& aZoneIndex, TUint* aPrefIndex = NULL)
	{
	// Get the most pref zone which has free pages
	GetPrefOrder();
	TUint prefIndex = 0;
	for (; prefIndex < gZoneCount; prefIndex++)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		if (gZoneUtilArray[zoneIndex].iFreePages)
			{
			aZoneIndex = zoneIndex;
			if (aPrefIndex)
				{
				*aPrefIndex = prefIndex;
				}
			return KErrNone;
			}
		}
	return KErrNotFound;
	}
//
// CanGenSucceed
//
// Check whether a call to TRamDefragRequest::DefragRam() would be able to 
// succeed or not. 
//
TBool CanGenSucceed()
	{
	GetPrefOrder();
	TBool genSucceed = EFalse;
	// Work out if general has anything to do
	for(TInt prefIndex = (TInt)gZoneCount-1; prefIndex >= 0; prefIndex--)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		TESTDEBUG(test.Printf(_L("prefIndex = %d zoneIndex = 0x%x\n"), prefIndex, zoneIndex));

		TUint samePrefCount = 1;
		TUint samePrefSucceed = 0;
		// The number of zones of this preference that will be skipped by the general defrag
		TUint samePrefEmptyImmovable = 0;

		// Determine how many zones have the same preference as this one
		TInt prevPrefIndex = (prefIndex != 0)? (prefIndex - 1) : -1;
		for (; prevPrefIndex >= 0; prevPrefIndex--)
			{
			TUint prevIndex = gPrefArray[prevPrefIndex];
			if (ZonesSamePref(zoneIndex, prevIndex))
				{
				samePrefCount++;
				}
			else // no more zones with this preference
				break;
			}
		TESTDEBUG(test.Printf(_L("samePrefCount = %d\n"), samePrefCount));

		for (TInt l = prefIndex - (samePrefCount-1); l <= prefIndex; l++)
			{
			TUint curPrefIndex = gPrefArray[l];
			TESTDEBUG(test.Printf(_L("curPrefIndex = %d\n"), curPrefIndex));
			if (gZoneUtilArray[curPrefIndex].iFreePages != gZoneConfigArray[curPrefIndex].iPhysPages)
				{
				TBool clearMovable = EFalse;
				TBool clearDiscardable = EFalse;

				if (gZoneUtilArray[curPrefIndex].iAllocUnknown || gZoneUtilArray[curPrefIndex].iAllocFixed)
					{
					TESTDEBUG(test.Printf(_L("unknown or fixed\n")));
					samePrefEmptyImmovable++;
					continue;
					}
				if (gZoneUtilArray[curPrefIndex].iAllocMovable)
					{// determine if movable can potentially be cleared from this zone
					TUint freeInLower = 0;
					for (TInt j=0; j <= prefIndex; j++)
						{
						TUint idx = gPrefArray[j];
						if (idx == curPrefIndex)
							continue;
						freeInLower += gZoneUtilArray[idx].iFreePages;
						}
					if (gZoneUtilArray[curPrefIndex].iAllocMovable <= freeInLower)
						{
						clearMovable = ETrue;
						TESTDEBUG(test.Printf(_L("Can clear movable, curPrefIndex = %d\n"), curPrefIndex));
						}
					}
				else
					{
					TESTDEBUG(test.Printf(_L("Can clear movable, curPrefIndex = %d\n"), curPrefIndex));
					clearMovable = ETrue;
					}
				if (gZoneUtilArray[curPrefIndex].iAllocDiscardable)
					{
					if (gPagedRom)
						{
						TUint minCacheSize = 0;
						TUint maxCacheSize = 0;
						TUint currentCacheSize = 0;

						DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
						TUint spareCache = currentCacheSize - minCacheSize;
						if (spareCache >= gZoneUtilArray[curPrefIndex].iAllocDiscardable )
							{
							clearDiscardable = ETrue;
							TESTDEBUG(test.Printf(_L("Paged: Can clear discardable\n")));
							}
						else
							{// determine space for discardable in more preferable zones
							TUint freeInLower = 0;
							for (TInt j=0; j <= prefIndex; j++)
								{
								TUint idx = gPrefArray[j];
								if (idx == curPrefIndex)
									continue;
								freeInLower += gZoneUtilArray[idx].iFreePages;
								}
							if (gZoneUtilArray[curPrefIndex].iAllocDiscardable - spareCache <= freeInLower)
								{
								clearDiscardable = ETrue;	
								TESTDEBUG(test.Printf(_L("Paged: Can clear discardable curPrefIndex = %d\n"), curPrefIndex));
								}
							}
						}
					else
						{//Should always be OK to discard as no min cache size on non-paged ROMS
						clearDiscardable = ETrue;
						test.Printf(_L("Can clear discardable curPrefIndex = %d\n"), curPrefIndex);
						}
					}
				else
					{
					clearDiscardable = ETrue;
					}

				if (clearDiscardable && clearMovable)
					{
					samePrefSucceed++;
					TESTDEBUG(test.Printf(_L("General should succeed ID=%x\n"), gZoneConfigArray[curPrefIndex].iZoneId));
					}
				}
			else
				{//zone already empty
				samePrefEmptyImmovable++;
				}
			}
		if (samePrefSucceed == 0 && samePrefEmptyImmovable == 0)
			{// no zones can be defragged and none are already empty/have immovable.
			break;
			}
		if (samePrefEmptyImmovable != samePrefCount)
			{// Have reached some zones with allocated pages in them.
			if (samePrefSucceed + samePrefEmptyImmovable == samePrefCount)
				{// general should definitely succeed as each of the zones of this preference 
				// can be emptied or are already empty/have immovable pages allocated.
				TESTDEBUG(test.Printf(_L("General should succeed \n")));
				genSucceed = ETrue;
				}
			break;
			}
		prefIndex -= samePrefCount - 1;
		}
	return genSucceed;
	}


//
// ReadByte
//
// Read a particular byte 
//
TUint8 ReadByte(volatile TUint8* aPtr)
	{
	return *aPtr;
	}


//
// AllocDiscardable
//
// Allocate Discardable pages in the form of demand paged pages
//
// @param aNumDiscardableBytes On return this will contain the number of discardable bytes above the min cache size.
// @param aMaxBytes The new limit for maximum number of bytes in the DP cache, set to KMaxTUInt64 to fill RAM.
// @param aMinOffsetBytes When not set to KMaxTUint64, this sets the min cache size to be the max cache size - aMinOffsetBytes.
//
TInt AllocDiscardable(TInt& aNumDiscardableBytes, TUint64 aMaxBytes = KMaxTUint64, TUint64 aMinOffsetBytes = KMaxTUint64)
	{
	TUint minCacheSize = 0;
	TUint maxCacheSize = 0;
	TUint currentCacheSize = 0;
	DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
	TESTDEBUG(test.Printf(_L("Original CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
								minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));

	TESTDEBUG(test.Printf(_L("SetCacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x\n"), minCacheSize >> gPageShift, aMaxBytes >> gPageShift));

	if (aMaxBytes == KMaxTUint64)
		{// Need to fill all of free memory with discardable pages
		UpdateRamInfo();
		maxCacheSize = minCacheSize + gFreeRam;
		if (aMinOffsetBytes != KMaxTUint64)
			{// Set the min cache size relative to the max cache size.
			minCacheSize = maxCacheSize - aMinOffsetBytes;
			}
		TESTDEBUG(test.Printf(_L("free 0x%x max 0x%x min 0x%x\n"), gFreeRam, maxCacheSize, minCacheSize));
		}
	else
		{
		maxCacheSize = aMaxBytes;
		}

	TInt r = DPTest::SetCacheSize(maxCacheSize, maxCacheSize);
	if (r != KErrNone)
		return r;
	r = DPTest::SetCacheSize(minCacheSize, maxCacheSize);
	if (r != KErrNone)
		return r;
	DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
	TESTDEBUG(test.Printf(_L("After CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
					minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));

	aNumDiscardableBytes = currentCacheSize - minCacheSize;
	TESTDEBUG(test.Printf(_L("Number of discardable bytes 0x%x\n"), aNumDiscardableBytes));

	if (aMaxBytes == KMaxTUint64)
		{
		UpdateRamInfo();
		if (gFreeRam != aNumDiscardableBytes)
			{// The only free RAM should be that of the DP cache.
			test.Printf(_L("gFreeRam 0x%x aNumDiscardableBytes 0x%x\n"), gFreeRam, aNumDiscardableBytes);
			return KErrGeneral;
			}
		}
	return KErrNone;
	}	
	

//
// ResetDPCache
//
// Flush the cache and set the boundaries back to their original values
//
void ResetDPCache()
	{
	if (gPagedRom)
		{
		TUint minCacheSize = 0;
		TUint maxCacheSize = 0;
		TUint currentCacheSize = 0;
		
		TESTDEBUG(test.Printf(_L("FlushCache\n")));
		TInt r = DPTest::FlushCache();

		DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
		TESTDEBUG(test.Printf(_L("After CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
						minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
		
		
		r = DPTest::SetCacheSize(gOriginalMinCacheSize, gOriginalMinCacheSize);
		r = DPTest::SetCacheSize(gOriginalMinCacheSize, gOriginalMaxCacheSize);
		TESTDEBUG(test.Printf(_L("SetCacheSize returns r = %d\n"), r));
		DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
		TESTDEBUG(test.Printf(_L("After CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
						minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
		}
	}
	
//
// WriteToChunk
//
// Write to a number of chunks
//
TInt WriteToChunk(RChunk* aChunkArray, TUint aChunkArraySize, TInt aChunk, TUint8 aStart=0)
	{
	for (TUint i=0; i<10; i++) // Write to all chunks 10 times
		{
		for (TUint j=0; j < aChunkArraySize; j++) //Write to all open chunks except aChunk.
			{
			if (aChunkArray[j].Handle() != NULL)
				{
				if ((TInt)j == aChunk) // Don't write to specified chunk
					{	
					continue;	
					}
				TUint8* base = aChunkArray[j].Base();
				for (TUint8 k = aStart; k < aStart + 10; k++)
					{
					*base++ = k; // write 0 - 9 to the chunk
					}
				}		
			}
		}
	return KErrNone;
	}


//
// ReadChunk
//
// Read chunks - If a chunk is specified, that particular chunk is not read
//
TInt ReadChunk(RChunk* aChunkArray, TUint aChunkArraySize, TInt aChunk=-1)
	{
	for (TUint j=0; j < aChunkArraySize; j++) //Read all open chunks
		{
		if (aChunkArray[j].Handle() != NULL)
			{
			if ((TInt)j == aChunk) // Don't read specified chunk
				{	
				continue;	
				}
			TUint8* base = aChunkArray[j].Base();
			while ((aChunkArray[j].Base() + aChunkArray[j].Size()) != base)
				{
				READ(base++);				
				}
			}		
		}
	return KErrNone;
	}
	

//
// RemoveChunkAlloc
//
// Remove ALL chunks allocated
//
// @param aChunkArray The array that stores a reference to the chunks created.
// @param aChunkArraySize The size of aChunkArray.
//
void RemoveChunkAlloc(RChunk*& aChunkArray, TUint& aChunkArraySize)
	{
	TInt closedChunks = 0;

	if (aChunkArray == NULL)
		{// The chunk array has already been deleted.
		return;
		}

	for (TUint i = 0; i < aChunkArraySize; i++)
		{
		if (aChunkArray[i].Handle() != NULL)
			{
			aChunkArray[i].Close();
			closedChunks ++;
			}
		}
	delete[] aChunkArray;
	aChunkArray = NULL;
	aChunkArraySize = 0;
	UpdateRamInfo();
	test.Printf(_L("Free RAM after closing %d chunks = 0x%x\n"),closedChunks,gFreeRam);
	}	


TBool SpaceAvailForPageTables(TUint aZoneIndex, TInt aNumPages)
	{
	// Every 1MB allocated needs a new page table
	const TUint KDataBytesPerPageTable = 1024 * 1024; 
	
	// 1 Page can fit 4 page tables
	const TUint KPageTablesPerPage = 4;
	
	
	GetAllPageInfo();
	if (aNumPages == KFillAllMovable)
		{
		aNumPages = gTotalPageCount.iFreePages;
		}

	TUint allocBytes = aNumPages << gPageShift;

	// Add 1 as you always require at least 1 page table
	TUint pageTablesRequired = (allocBytes / KDataBytesPerPageTable) + 1;
	// Add 1 as the first 1-3 page tables may require a new page.
	TUint pageTablePagesRequired = (pageTablesRequired / KPageTablesPerPage) + 1;
	
	// Determine the number of free pages in the other zones
	TUint freeInOther = 0;
	for (TUint index = 0; index < gZoneCount; index++)
		{
		if (index != aZoneIndex)
			{
			freeInOther += gZoneUtilArray[index].iFreePages;
			}
		}

	// Need an overhead for the heap to grow (5 pages)
	const TUint KOverhead = 5;
	if (freeInOther < pageTablePagesRequired + KOverhead)
		{// Not enough space in other zones to fit all page tables
		test.Printf(_L("No space in other zones for page table pages\n"));
		return EFalse;
		}
	// There is space available in the other zones to fit all the page tables
	return ETrue;
	}
//
// AllocMovable
//
// Allocate movable memory in the form of chunks
//
// @param aChunkArray The array to store a reference to the chunks created.
// @param aChunkArraySize The size of aChunkArray.
// @param aNumChunks The number of chunks to create.
// @param aNumPages The size of each chunk.
//
TInt AllocMovable(RChunk*& aChunkArray, TUint& aChunkArraySize, TInt aNumChunks, TUint aChunkSize=KChunkDefaultSize, TBool aForceFill = ETrue)
	{
	TUint i = 0;
	TInt r = 0;
	TUint chunksAllocd = 0;
	TBool fillAll = EFalse;

	TInt numChunks = aNumChunks;
	UpdateRamInfo();
	// Allocate chunks to take up all of memory with the maximum number of chunks
	if (aNumChunks == KFillAllMovable)
		{
		fillAll = ETrue;
		if (aChunkSize == KChunkDefaultSize)
			{
			numChunks = KMaxChunks;
			aChunkSize = gFreeRam / numChunks; 
			}
		else
			{
			numChunks = gFreeRam  / aChunkSize;
			}
		}

	test.Printf(_L("gFreeRam = 0x%x, aChunkSize = 0x%x, numChunks = %d\n"), gFreeRam, aChunkSize, numChunks);
	
	// Allocate as many chunks as is specified, either with the default chunk size or a specified chunk size
	if (aChunkArray == NULL)
		{
		aChunkArraySize = numChunks;
		aChunkArray = new RChunk[aChunkArraySize];
		if (aChunkArray == NULL)
			return KErrNoMemory;
		}

	// Create chunks for each RChunk with a NULL handle.
	for (i = 0; i < aChunkArraySize; i++)
		{
		if (aChunkArray[i].Handle() == NULL)
			{
			// Keep going even if a chunk creation fails as the flag tests rely 
			// on this.
			r = aChunkArray[i].CreateLocal(aChunkSize, aChunkSize);
			if (r != KErrNone && fillAll && aForceFill)
				{
				while (aChunkArray[i].CreateLocal(aChunkSize, aChunkSize) != KErrNone)
					{
					aChunkSize -= gPageSize;
					}
				}
			if (r == KErrNone)	
				{
				chunksAllocd++;
				}
			}
		User::After(10); // Wait so that the next chunk gets allocated in the next time slice
		}
	test.Printf(_L("Number of chunks allocd = %d\n"),chunksAllocd);
	return r;
	}

//
// ZoneAllocMovable
//
// Allocate the specified number of movable pages to a specific zone
// If the number of pages is not specified, then fill the specified zone with
// movable pages
//
// @param aChunkArray The array to store a reference to the chunks created.
// @param aChunkArraySize The size of aChunkArray.
// @param aZoneIndex The zone index to allocate movable pages to.
// @param aNumPages The number of movable pages to allocate.
//
TInt ZoneAllocMovable(RChunk*& aChunkArray, TUint& aChunkArraySize, TUint aZoneIndex, TUint aNumPages = KMaxTUint)
	{
	ResetRamZoneFlags();
	TInt r = KErrNone;
	TUint allocBytes = 0;

	if (aNumPages == KMaxTUint)
		{
		aNumPages = gZoneUtilArray[aZoneIndex].iFreePages;
		}

	allocBytes = aNumPages << gPageShift;

	if (!SpaceAvailForPageTables(aZoneIndex, aNumPages))
		{
		return KErrGeneral;
		}

	// Block all other zones from allocation
	for(TUint index = 0; index < gZoneCount; index++)
		{
		if (index == aZoneIndex)
			{
			r = Ldd.SetZoneFlag(gZoneConfigArray[index].iZoneId, gZoneConfigArray[index].iFlags, NO_FIXED_FLAG);
			}
		else
			{
			r = Ldd.SetZoneFlag(gZoneConfigArray[index].iZoneId, gZoneConfigArray[index].iFlags, NO_MOVE_FLAG);
			}
		if (r != KErrNone)
			{
			test.Printf(_L("Failed to set RAM zone flag on zone %d r = %d\n"), index, r);
			return r;
			}
		}

	// Allocate the movable pages
	r = AllocMovable(aChunkArray, aChunkArraySize, 1, allocBytes);

	ResetRamZoneFlags();
	return r;
	}

//
// ZoneAllocDiscard
//
// Allocate the specified number of discardable pages to a specific zone
//
// @param aZoneIndex The zone index to allocate discardable pages to.
// @param aNumPages The number of discardable pages to allocate.
// @param aDisPages On return this will contain the number of discardable pages allocated
//
TInt ZoneAllocDiscard(TUint aZoneIndex, TUint aNumPages, TInt& aDisPages)
	{
	TInt r = KErrNone;
	ResetRamZoneFlags();
	
	for (TUint index = 0; index < gZoneCount; index++)
		{
		TUint zoneID = gZoneConfigArray[index].iZoneId;
		if (index == aZoneIndex)
			{
			r = Ldd.SetZoneFlag(zoneID, gZoneConfigArray[index].iFlags, ONLY_DISCARD_FLAG);
			}
		else
			{
			r = Ldd.SetZoneFlag(zoneID, gZoneConfigArray[index].iFlags, NO_DISCARD_FLAG);
			}

		if (r != KErrNone)
			{			
			test.Printf(_L("Failed to set flag r = %d\n"), r);
			return r;
			}
		}
	
	TUint disBytes = gTotalPageCount.iDiscardablePages + aNumPages << gPageShift;
	r = AllocDiscardable(aDisPages, disBytes);
	aDisPages = aDisPages >> gPageShift;
	if (r != KErrNone)
		{
		test.Printf(_L("Discardable pages not allocated r = %d\n"), r);
		}
	ResetRamZoneFlags();
	return r;
	}

//
// FreeMovable
//
// Free movable pages by closing chunks. 
// The function will close every other chunk so that the movable pages are scattered in every zone
//
// @param aChunkArray The array that stores reference to the chunks created.
// @param aChunkArraySize The size of aChunkArray.
//
TInt FreeMovable(RChunk* aChunkArray, TUint aChunkArraySize)
	{
	TUint i;
	TInt closedChunks = 0;
		
	for (i=0; i < aChunkArraySize; i+=2) // Close every other chunk
		{
		if (aChunkArray[i].Handle() != NULL)
			{
			aChunkArray[i].Close();
			closedChunks ++;
			}
		}
	UpdateRamInfo();
	test.Printf(_L("Free RAM after closing %d chunks = 0x%x\n"),closedChunks,gFreeRam);
	return KErrNone;
	}

//
// GetBestZone
//
// Obtains the most preferable zone for allocating a specific type of page
//
// @param aPageType The page type that we are interested in
// @param aBestPrefIndex The index into the preference array for the zone.
//
// @return KErrNotFound if a zone cannot be found, else the zone index into gZoneUtilArray
//
TInt GetBestZone(TInt aPageType, TUint* aBestPrefIndex = NULL)
	{
	GetPrefOrder();


	switch (aPageType)
		{
		case BEST_MOVABLE:			
		case BEST_DISCARDABLE:
			{
			TInt bestIndex = KErrNotFound;
			TUint bestPrefIndex = 0;
			TUint curIndex = 0;
			TInt startIndex = KErrNotFound;
			TBool zoneFound = EFalse;
			// Find the least preferable zone that has movable or discardable pages
			for (TInt prefIndex = gZoneCount - 1; prefIndex >= 0; prefIndex--)
				{
				curIndex = gPrefArray[prefIndex];
				if (gZoneUtilArray[curIndex].iAllocMovable || gZoneUtilArray[curIndex].iAllocDiscardable)
					{
					startIndex = prefIndex;
					break;
					}
				}
			if (startIndex == KErrNotFound)
				return startIndex;
				
			// Work up the preference list to look for the best zone
			for (TInt prefIndex = startIndex; prefIndex >= 0; prefIndex--)
				{
				curIndex = gPrefArray[prefIndex];
				if (gZoneUtilArray[curIndex].iFreePages)
					{
					bestIndex = curIndex;
					bestPrefIndex = prefIndex;
					zoneFound = ETrue;
					break;
					}
				}
			
			// If the zone still isn't found, look down the preference list
			if (!zoneFound)
				{
				for (TUint prefIndex = startIndex; prefIndex < gZoneCount; prefIndex++)
					{
					curIndex = gPrefArray[prefIndex];
					if (gZoneUtilArray[curIndex].iFreePages)
						{
						bestIndex = curIndex;
						bestPrefIndex = prefIndex;
						zoneFound = ETrue;
						break;
						}
					}
				}
				
			test.Printf(_L("leastPref = %d\n"), bestIndex);
			if (aBestPrefIndex)
				*aBestPrefIndex = bestPrefIndex;
			return bestIndex;
			}

		case BEST_FIXED:
			for (TUint prefIndex = 0; prefIndex < gZoneCount; prefIndex++)
				{
				TUint mostPref = gPrefArray[prefIndex];
				if (gZoneUtilArray[mostPref].iFreePages != 0 || 
					gZoneUtilArray[mostPref].iAllocMovable != 0 ||
					gZoneUtilArray[mostPref].iAllocDiscardable != 0)
					{
					test.Printf(_L("mostPref = %d\n"), mostPref);
					if (aBestPrefIndex)
						*aBestPrefIndex = prefIndex;
					return mostPref;
					}
				}
			break;
		}
	
	test.Printf(_L("Cannot find zone\n"));
	return KErrNotFound;
	}

void GetCandList1(TInt aIndex);
//
// VerifyMovDisAlloc
//
// Checks that all movable and discardable pages are in the correct RAM zones.
// Should only be invoked after a general defragmentation or a general 
// defragmentation followed by an allocation.
//
//	NOTE - This shouldn't be used to verify RAM if a RAM specific allocation or
//	zone claim operation has been performed.
//
// @return KErrNone if RAM layout is good, KErrGeneral if not, KErrNotFound if no
// movable or discardable pages are allocated.
//
TInt VerifyMovDisAlloc()
	{
	GetPrefOrder();
	TInt leastInUse = KErrNotFound;
	TUint requiredMovDis = 0;
	TUint totalMorePrefInUse = 0;
	TBool verifySpread = ETrue;
	TBool prevZoneNotFull = EFalse;

	// Determine which is the least prefable RAM zone in use and how many pages
	// are allocated of each type.
	for (TInt prefIndex = gZoneCount - 1; prefIndex >= 0; prefIndex--)
		{
		TUint index = gPrefArray[prefIndex];
		TUint allocMov = gZoneUtilArray[index].iAllocMovable;
		TUint allocDis = gZoneUtilArray[index].iAllocDiscardable;
		TUint allocFixed = gZoneUtilArray[index].iAllocFixed;
		TESTDEBUG(test.Printf(_L("pref ID 0x%x\n"), gZoneConfigArray[index].iZoneId));
		if (allocMov || allocDis || allocFixed)
			{
			TBool zoneNotFull = EFalse;
			GetCandList1(index);
			TUint candIndex = 0;
			for (; candIndex < gZoneCount; candIndex++)
				{
				TInt zoneIndexCand = gCandList1[candIndex];
				if (zoneIndexCand == KInvalidCandIndex)
					{
					break;
					}
				allocMov  += gZoneUtilArray[zoneIndexCand].iAllocMovable;
				allocDis  += gZoneUtilArray[zoneIndexCand].iAllocDiscardable;
				if (gZoneUtilArray[zoneIndexCand].iFreePages)
					{
					zoneNotFull = ETrue;
					}
				}
			prefIndex -= candIndex - 1;
			if (leastInUse == KErrNotFound)
				{// Have found least preferable RAM zone that is in use.
				leastInUse = index;
				if (allocFixed)
					{// The least preferable RAM zone has fixed pages in it so
					// RAM may be more spread out than is necessary.
					verifySpread = EFalse;
					}
				}
			else
				{
				if (verifySpread && (allocMov || allocDis) && prevZoneNotFull)
					{// The previous least preferable RAM zones were not full so shouldn't
					// be any movable or discardable pages in the RAM zones of this preference.
					test.Printf(_L("Movable or discardable pages in more preferable RAM zones unnecessarily\n"));
					return KErrGeneral;
					}
				prevZoneNotFull = zoneNotFull;

				// Not the least preferable RAM zone so add to total allocatable.
				totalMorePrefInUse += allocMov + allocDis + gZoneUtilArray[index].iFreePages;
				}
			requiredMovDis += allocMov + allocDis;
			}
		}

	if (leastInUse == KErrNotFound)
		{// No movable or discardable pages are allocated.
		test.Printf(_L("No in use RAM zones found????\n"));
		return KErrNotFound;
		}

	if (totalMorePrefInUse > requiredMovDis)
		{// There are enough allocatable pages in the RAM zones below the currently 
		// least preferable RAM in use.
		test.Printf(_L("Memory is spread out totalMorePref 0x%x required 0x%x\n"), totalMorePrefInUse, requiredMovDis);
		if (verifySpread)
			return KErrGeneral;
		}
	return KErrNone;
	}


//
// GetCandList1
//
// Populates a list of all zones that have the same preference and the same amount 
// of immovable pages
//
void GetCandList1(TInt aIndex)
	{
	for (TUint i=0; i<gZoneCount; i++)
		{
		gCandList1[i] = KInvalidCandIndex;
		}

	TInt candListIndex = 0;
	GetAllPageInfo();
	for (TUint i=0; i<gZoneCount; i++)
		{
		if (ZonesSamePref(aIndex, i))
			{
			gCandList1[candListIndex] = i;
			candListIndex ++;
			}
		}
	}

//
// GetCandList2
//
// Populates a list of all zones that have the same preference and the same amount 
// of immovable pages
//
void GetCandList2(TInt aIndex)
	{
	for (TUint i=0; i<gZoneCount; i++)
		{
		gCandList2[i] = KInvalidCandIndex;
		}

	TInt candListIndex = 0;
	GetAllPageInfo();
	for (TUint i=0; i<gZoneCount; i++)
		{
		if (ZonesSamePref(aIndex, i))
			{
			gCandList2[candListIndex] = i;
			candListIndex ++;
			}
		}
	}

//
// GetCandListFixed1
//
// Populates a list of all zones that have the same preference and the same 
// amount of immovable pages, it will ignore RAM zones that are full of 
// immovable pages as these are the only RAM zones that fixed pages can't be 
// allocated into.
//
void GetCandListFixed1(TInt aIndex)
	{
	for (TUint i=0; i<gZoneCount; i++)
		{
		gCandList1[i] = KInvalidCandIndex;
		}

	TInt candListIndex = 0;
	GetAllPageInfo();
	for (TUint i=0; i<gZoneCount; i++)
		{
		if (ZonesSamePref(aIndex, i) &&
			gZoneUtilArray[i].iAllocFixed + gZoneUtilArray[i].iAllocUnknown !=
			gZoneConfigArray[i].iPhysPages) 
			{
			gCandList1[candListIndex] = i;
			candListIndex ++;
			}
		}
	}

//
// MultiGenDefragThreadFunc
//
// Called when a general defrag is called to run at the same time as another operation
//
TInt MultiGenDefragThreadFunc(TAny* /*aPtr*/)
	{
	RRamDefragFuncTestLdd Ldd2;
	TInt r = Ldd2.Open();
	if (r != KErrNone)
		{
		RDebug::Printf("Unable to open Ldd2 in MultiGenDefragThreadFunc, r = %d\n", r);
		return r;
		}

	RThread thisThread = RThread();
	thisThread.SetPriority(EPriorityLess);
	r = Ldd2.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	Ldd2.Close();
	thisThread.Close();
	return r;
	}

TBool gTestLoop = EFalse;
//
// MultiLoopGenDefragThreadFunc
//
// Called when a general defrag is called to run at the same time as another operation
//
TInt MultiLoopGenDefragThreadFunc(TAny* /*aPtr*/)
	{
	RRamDefragFuncTestLdd Ldd2;
	TInt r = Ldd2.Open();
	if (r != KErrNone)
		{
		RDebug::Printf("Unable to open Ldd2 in MultiLoopGenDefragThreadFunc, r = %d\n", r);
		return r;
		}

	RThread thisThread = RThread();
	thisThread.SetPriority(EPriorityLess);
	while (gTestLoop)
		{
		r = Ldd2.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		if (r != KErrNone)
			goto threadExit;
		}
threadExit:
	Ldd2.Close();
	thisThread.Close();
	return r;
	}

//
// MultiEmptyZoneThreadFunc
//
// Called when a zone defrag is called to run at the same time as another operation
//
TInt MultiEmptyZoneThreadFunc(TAny* zoneID)
	{
	RRamDefragFuncTestLdd Ldd2;
	TInt r = Ldd2.Open();
	if (r != KErrNone)
		{
		RDebug::Printf("Unable to open Ldd2 in MultiEmptyZoneThreadFunc, r = %d\n", r);
		return r;
		}

	RThread thisThread = RThread();
	thisThread.SetPriority(EPriorityLess);
	r = Ldd2.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, (TUint)zoneID);
	Ldd2.Close();
	thisThread.Close();
	return r;
	}


//
// MultiClaimZoneThreadFunc
//
// Called when a ClaimRamZone is called to run at the same time as another operation
//
TInt MultiClaimZoneThreadFunc(TAny* zoneID)
	{
	RRamDefragFuncTestLdd Ldd2;
	TInt r = Ldd2.Open();
	if (r != KErrNone)
		{
		RDebug::Printf("Unable to open Ldd2 in MultiClaimZoneThreadFunc, r = %d\n", r);
		return r;
		}

	RThread thisThread = RThread();
	thisThread.SetPriority(EPriorityLess);
	r = Ldd2.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, (TUint)zoneID);
	Ldd2.Close();
	thisThread.Close();
	return r;
	}


//
// TestAllocStrategies
//
// Verifying that pages are allocated correctly.  All tests rely on a general defragmentation occuring
// in TestStart() so that memory is in a defragmented state before the allocations.
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0525
//! @SYMTestCaseDesc			Verifying that pages are allocated correctly
//! @SYMTestType				CIT
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Allocate one chunk to memory. 
//! 		Check which zone the chunk has been allocated to by checking the 
//! 		number of movable pages before and after the allocation has taken place. 
//!		2.	Allocate one chunk to memory that is large enough to cause an empty RAM 
//!			zone to be used.
//! 	3.	Allocate a certain number of fixed pages to memory. 
//! 		Check which zone the chunk has been allocated to by checking the 
//! 		number of fixed pages before and after the allocation has taken place. 
//!		4.	Allocate fixed pages when it is known that discardable pages will need 
//!			to be discarded for the allocation to succeed.
//!		5.	Allocate fixed pages when it is known that movable pages will need 
//!			to be moved for the allocation to succeed.
//! 	6.	Allocate fixed pages when it is known that movable pages will need 
//!			to be moved for the allocation to succeed, Determine the "best" zone for fixed pages and allocate 1 more
//!			than that. 
//!		7.	Allocate discardable pages by loading pages that are demand paged. 
//! 		Check which zone the memory has been allocated to by checking the number of 
//! 		discardable pages before and after the allocation has taken place. 
//!		8.	Allocate a contiguous fixed page into a zone which is full of movable or discardable pages.
//!		9.	Allocate a large 1MB aligned chunk with as few pages as possible availiable for page tables,
//!			then clear to every page in the chunk.  This is attempting to ensure that the chunk pages 
//!			that have page tables associated to them are remapped correctly if the chunk pages have to be
//!			moved so page table pages to map other pages in the chunk are allocated in the most preferable 
//! 		ram zones.
//! 
//! @SYMTestExpectedResults
//! 	1.	The memory has been allocated to the most preferred zone with the least amount of 
//! 		space accounting for the zone threshold for movable pages. 
//!		2.	The new RAM zone is used and the movable page are allocated into it first then the other
//!			more preferable RAM zones.
//! 	3.	The fixed pages are allocated to the most preferred zone
//!		4.	The fixed pages are allocatted to the most preferred zone with free, movable or discardable pages in it.
//!		5.	The fixed pages are allocatted to the most preferred zone with free, movable or discardable pages in it.
//! 	6.	Extra page is placed in the next preferable to the "best"zone 
//!		7.	Memory is allocated to the most preferred zone with the least amount of 
//! 		space accounting for the zone threshold for discardable pages. 
//!		8.	The fixed page is allocated as it has moved or discarded a page.
//!		9.	The chunk is cleared succesfully.
//!
//---------------------------------------------------------------------------------------------------------------------
TInt TestAllocStrategies()
	{
	
	test.Start(_L("Test1: Check which zone a movable page has been allocated to "));	
	TestStart(); //This will perform a general defrag which should tidy up RAM for us
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	TBool zoneFound = EFalse;

	// Determine if suitable RAM zone exists for testing
	TInt best = GetBestZone(BEST_MOVABLE);
	if (best == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping...\n"));
		}
	else
		{
		test.Printf(_L("best = %d\n"), best);
		// Populate the candidate list
		GetCandList2(best);
		// Allocate 1 movable page
		GetOriginalPageCount();
		if (AllocMovable(gChunkArray1, gChunkArraySize1, 1, gPageSize) != KErrNone)
			{
			test.Printf(_L("Not enough free RAM for test - Skipping...\n"));
			}
		else
			{
			GetAllPageInfo();
			TInt r = VerifyMovDisAlloc();
			// Need to check all candidates to see if page has gone into any one of them
			for (TUint i=0; i < gZoneCount; i++)
				{
				if (gCandList2[i] == KInvalidCandIndex)
					{
					break;
					}
				TUint zoneIndex = gCandList2[i];
				if (gOriginalPageCountArray[zoneIndex].iAllocMovable < gZoneUtilArray[zoneIndex].iAllocMovable)
					{
					zoneFound = ETrue;
					break;
					}
				}

			if (r == KErrNone && zoneFound)
				{
				test.Printf(_L("Pass: Movable allocated to the zone expected\n"));
				}
			else
				{
				test.Printf(_L("Fail: Movable allocated to a zone that was not expected, r %d\n"), r);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}	
			}
		}
	TestEnd();

	test.Next(_L("Test2: Check that movable pages allocated from least preferable RAM zone to be used\n"));
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	// Determine if suitable RAM zone exists for testing.
	TUint bestPrefIndex;
	best = GetBestZone(BEST_MOVABLE, &bestPrefIndex); 
	TUint morePrefIndex = bestPrefIndex - 1;
	TUint nextLeastPrefIndex = bestPrefIndex + 1;
	

	// Check number of free pages in the more preferable zones
	TUint freeInMorePref = 0;
	for (TUint i = 0; i < bestPrefIndex; i++)
		{
		freeInMorePref += gZoneUtilArray[gPrefArray[i]].iFreePages;
		}
	
	test.Printf(_L("freeInMorePref = 0x%x\n"), freeInMorePref);
	const TUint KHeapOverhead = 5; // Room for kernel heap allocations so they don't affect the page count
	if (best == KErrNotFound || nextLeastPrefIndex >= gZoneCount ||
		(gZoneUtilArray[gPrefArray[nextLeastPrefIndex]].iFreePages < KHeapOverhead && 
		freeInMorePref < KHeapOverhead)||
		gZoneConfigArray[gPrefArray[nextLeastPrefIndex]].iPref == gZoneConfigArray[best].iPref ||
		gZoneConfigArray[gPrefArray[morePrefIndex]].iPref == gZoneConfigArray[best].iPref)
		{// No less preferable RAM zone or there are more or less preferable of
		// same preference so re-ordering potential makes verification too complex.
		test.Printf(_L("Cannot find zone to perform test - Skipping...\n"));
		}
	else
		{
		// Ensure the zone is almost full as chunks will get allocated in blocks
		// by almost filling the best zone with fixed pages, leaving a space (e.g. 20 pages)
		const TUint KSpaceNeeded = 20;
		if (gZoneUtilArray[best].iFreePages > KSpaceNeeded)
			{
			TUint zoneID = gZoneUtilArray[best].iZoneId;
			TUint diffAlloc = gZoneUtilArray[best].iFreePages - KSpaceNeeded;
			TInt r = Ldd.ZoneAllocDiscontiguous(zoneID, diffAlloc);
			if (r != KErrNone)
				{
				CLEANUP(Ldd.FreeAllFixedPages());
				TEST_FAIL;
				}
			}
		

		
		GetAllPageInfo();
		// Update the number of free pages in the more preferable zones
		freeInMorePref = 0;
		for (TUint i = 0; i < bestPrefIndex; i++)
			{
			freeInMorePref += gZoneUtilArray[gPrefArray[i]].iFreePages;
			}
		TUint origFixed = gTotalPageCount.iFixedPages;

		GetOriginalPageCount();

		// Allocate enough movable pages that the next least preferable RAM zone 
		// will need to be used.
		TUint movPages = gZoneUtilArray[best].iFreePages + 1;
		TUint movBytes = movPages << gPageShift;
		TInt r = AllocMovable(gChunkArray1, gChunkArraySize1, 1, movBytes);
		if (r != KErrNone)
			{
			CLEANUP(Ldd.FreeAllFixedPages());
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}

		GetAllPageInfo();
		TUint curFixed = gTotalPageCount.iFixedPages;
		TInt difFixed = curFixed - origFixed;	
		
		// If there is space in the more preferable zones then they should be allocated starting from
		// the "best" zone to the more preferable zones, else from the next least preferable zone to the more preferable zones
		TUint prefIndex;		
		if ((TInt)freeInMorePref > difFixed)
			{//No new zones should be turned on, allocation should have gone into bestPrefIndex then down towards most preferred zones
			TUint nextIndex = gPrefArray[nextLeastPrefIndex];
			if (gOriginalPageCountArray[best].iAllocMovable >= gZoneUtilArray[best].iAllocMovable ||
				gOriginalPageCountArray[nextIndex].iAllocMovable < gZoneUtilArray[nextIndex].iAllocMovable)
				{
				test.Printf(_L("Movable page allocated incorrectly origFree 0x%x curFree 0x%x nxtPrefFree 0x%x\n"), 
								gOriginalPageCountArray[best].iFreePages, gZoneUtilArray[best].iFreePages, 
								gZoneUtilArray[nextLeastPrefIndex].iFreePages);
				CLEANUP(Ldd.FreeAllFixedPages());
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}
			prefIndex = morePrefIndex;
			}
		else
			{// If there are enough free pages in "nextIndex" to fit all movable pages allocated then the movable 
			// page count in "best" should stay the same, else they should be allocated between "nextIndex" and "best".
			TUint nextIndex = gPrefArray[nextLeastPrefIndex];
			test.Printf(_L("nextIndex = %d\n"), nextIndex);
			if (gOriginalPageCountArray[nextIndex].iAllocMovable >= gZoneUtilArray[nextIndex].iAllocMovable ||
				(gOriginalPageCountArray[nextIndex].iFreePages >= movPages && 
				gOriginalPageCountArray[best].iAllocMovable != gZoneUtilArray[best].iAllocMovable) ||
				(gOriginalPageCountArray[nextIndex].iFreePages < movPages && 
				gOriginalPageCountArray[best].iAllocMovable == gZoneUtilArray[best].iAllocMovable))
				{
				test.Printf(_L("Movable page allocated incorrectly origFree 0x%x curFree 0x%x nxtPrefFree 0x%x\n"), 
								gOriginalPageCountArray[nextIndex].iFreePages, gZoneUtilArray[nextIndex].iFreePages, 
								gZoneUtilArray[nextIndex].iFreePages);
				CLEANUP(Ldd.FreeAllFixedPages());
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}
			prefIndex = bestPrefIndex;
			}
		
		// Check that movable pages have only been allocated into the more 
		// preferable RAM zones if the less preferable ram zones in use are full.
		prefIndex++;
		do
			{
			prefIndex--;
			TUint indexCurrent = gPrefArray[prefIndex];			
			TUint indexLessPref = gPrefArray[prefIndex+1];

			if (gOriginalPageCountArray[indexCurrent].iAllocMovable < gZoneUtilArray[indexCurrent].iAllocMovable &&
				gZoneUtilArray[indexLessPref].iFreePages)
				{// Current in use zone or less preferable than current has free pages so fail
				test.Printf(_L("Movable page allocated incorrectly origFree 0x%x curFree 0x%x nxtPrefFree 0x%x\n"), 
								gOriginalPageCountArray[best].iFreePages, gZoneUtilArray[best].iFreePages, 
								gZoneUtilArray[nextLeastPrefIndex].iFreePages);
				CLEANUP(Ldd.FreeAllFixedPages());
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}
			}
		while(prefIndex);
		test.Printf(_L("Pass: Pages allocated to the zone expected\n"));
		}
	TestEnd();

	test.Next(_L("Test3: Check which zone a fixed page has been allocated to "));	
	for (TUint testStep = 0; testStep < 2; testStep++)
		{
		switch (testStep)
			{
			case 0:
				test.Printf(_L("Testing discontiguous allocations\n"));
				break;
				
			case 1:
				test.Printf(_L("Testing contiguous allocations\n"));
				break;
			}
		TestStart();
		zoneFound = EFalse;
		GetOriginalPageCount();
		
		best = GetBestZone(BEST_FIXED);
		TESTDEBUG(test.Printf(_L("best = %d\n"), best));
		if (best == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping...\n"));
			}
		else
			{
			GetCandList1(best);
			TInt allocFixedPages = 1; // Try and allocate just 1 fixed page
			switch (testStep)
				{
				case 0:
					Ldd.AllocateFixed(allocFixedPages); 
					break;
					
				case 1:
					TUint allocFixedBytes = allocFixedPages << gPageShift;
					Ldd.AllocContiguous(allocFixedBytes); 
					break;
				}
			
			
			GetAllPageInfo();
			for (TUint i=0; i<gZoneCount; i++)
				{
				if (gCandList1[i] == KInvalidCandIndex)
					{
					break;
					}
				TUint zoneIndex = gCandList1[i];
				if (gOriginalPageCountArray[zoneIndex].iAllocFixed + allocFixedPages <= gZoneUtilArray[zoneIndex].iAllocFixed)
					{
					zoneFound = ETrue;
					break;
					}
				}

			if (zoneFound)
				{
				test.Printf(_L("Pass: Chunk has been allocated to the zone expected\n"));
				}
			else
				{
				test.Printf(_L("Fail: Fixed been allocated to a zone that was not expected\n"));
				CLEANUP(Ldd.FreeAllFixedPages());
				TEST_FAIL;
				}	
			
			Ldd.FreeAllFixedPages();
			}
		TestEnd();
		}

	test.Next(_L("Test4: Check fixed page allocations will discard pages"));
	for (TUint testStep = 0; testStep < 2; testStep++)
		{
		switch (testStep)
			{
			case 0:
				test.Printf(_L("Testing discontiguous allocations\n"));
				break;
				
			case 1:
				test.Printf(_L("Testing contiguous allocations\n"));
				break;
			}
		TestStart();
		TInt discardBytes;
		TInt r;

		if (!gPagedRom)
			{
			test.Printf(_L("Not a paged ROM - Skipping test step\n"));
			goto SkipTest4;
			}

		best = GetBestZone(BEST_FIXED);
		TESTDEBUG(test.Printf(_L("best = %d\n"), best));
		if (best == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping...\n"));
			goto SkipTest4;
			}
		// Ensure discardable pages in the preferred RAM zone.
		r = AllocDiscardable(discardBytes);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			TEST_FAIL;
			}
		if (discardBytes < gPageSize)
			{// Can't discard any pages so test can't run.
			test.Printf(_L("Memory too full to perform test - Skipping...\n"));
			goto SkipTest4;
			}

		// Make sure all RAM zones that the fixed page allocation could potentially 
		// go to have discardable pages allocated in it.
		GetOriginalPageCount();
		GetCandListFixed1(best);
		for (TUint i = 0; i < gZoneCount; i++)
			{
			if (gCandList1[i] == KInvalidCandIndex)
				{
				break;
				}
			TUint zoneIndex = gCandList1[i];
			if (gOriginalPageCountArray[zoneIndex].iAllocDiscardable == 0)
				{
				test.Printf(_L("No dicardable pages in one of the candidates RAM zones - Skipping...\n"));
				goto SkipTest4;
				}
			if (gOriginalPageCountArray[zoneIndex].iFreePages != 0)
				{
				test.Printf(_L("Some free pages in candidate RAM zone ID%x\n"), 
							gZoneConfigArray[zoneIndex].iZoneId);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			}

		// Allocate 1 fixed page and ensure that it discards a page.
		switch (testStep)
			{
			case 0:
				r = Ldd.AllocateFixed(1); 
				break;
				
			case 1:
				r = Ldd.AllocContiguous(gPageSize); 
				break;
			}

		// Find RAM zone the fixed page was allocated into.
		GetAllPageInfo();
		zoneFound = EFalse;
		for (TUint i = 0; i < gZoneCount; i++)
			{
			if (gCandList1[i] == KInvalidCandIndex)
				{
				break;
				}
			TUint zoneIndex = gCandList1[i];
			if (gOriginalPageCountArray[zoneIndex].iAllocFixed < gZoneUtilArray[zoneIndex].iAllocFixed)
				{
				zoneFound = ETrue;
				if (gOriginalPageCountArray[zoneIndex].iAllocDiscardable <= gZoneUtilArray[zoneIndex].iAllocDiscardable &&
					gOriginalPageCountArray[zoneIndex].iAllocMovable <= gZoneUtilArray[zoneIndex].iAllocMovable)
					{
					test.Printf(_L("Fixed pages allocated but no pages discarded in RAM zone ID 0x%x\n"), 
						gZoneConfigArray[zoneIndex].iZoneId);
					CLEANUP(ResetDPCache());
					TEST_FAIL;
					}
				}
			}

		if (!zoneFound || r != KErrNone)
			{
			test.Printf(_L("No fixed pages were allocated r = %d\n"), r);
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Pass: Pages been allocated to the zone expected\n"));
			}
	SkipTest4 :
		// This will free any DP cache pages and fixed pages allocated.
		TestEnd();
		}


	for (TUint testIndex = 0; testIndex < 2; testIndex++)
		{
		switch (testIndex)
			{
			case 0:
				test.Next(_L("Test5: Check fixed page allocations (discontiguous) will move pages"));
				break;

			case 1:
				test.Next(_L("Test6: Check fixed page will only go into a new zone if all other fix page zones are full of fix"));
				break;
			}

		TestStart();
		gChunkArray1 = new RChunk;
		gChunkArraySize1 = 1;
		TInt r = KErrNone;
		TUint freeInOtherZones = 0;
		TUint allocatablePages;
		TUint allocImmovPages;
		TUint bestPrefIndex;
		TUint nextBestIndex = 0;
		const TUint KMovAllocOverhead = 5;	// need pages for page tables and kernel heap expansion.
		
		best = GetBestZone(BEST_FIXED, &bestPrefIndex);
		TESTDEBUG(test.Printf(_L("best = %d\n"), best));
		GetCandListFixed1(best);
		TUint candidates = 0;
		for (TUint i = 0; i < gZoneCount; i++)
			{
			if (gCandList1[i] == KInvalidCandIndex)
				{
				break;
				}
			candidates++;
			}
		if (best == KErrNotFound || 
			(gZoneUtilArray[best].iAllocMovable == 0 && gZoneUtilArray[best].iFreePages < KMovAllocOverhead) ||
			candidates != 1)
			{
			test.Printf(_L("Cannot find zone or too many equal pref zones to perform test - Skipping...\n"));
			goto SkipTest5;
			}

		if (testIndex == 1)
			{// need to work out what the next best zone would be
			GetPrefOrder();
			if (bestPrefIndex + 1 >= gZoneCount)
				{
				test.Printf(_L("Cannot find next best zone - Skipping...\n"));
				goto SkipTest5;
				}
			nextBestIndex = gPrefArray[bestPrefIndex + 1];
			test.Printf(_L("nextBestIndex= %d\n"), nextBestIndex);
			GetCandListFixed1(nextBestIndex);
			candidates = 0;
			for (TUint i = 0; i < gZoneCount; i++)
				{
				if (gCandList1[i] == KInvalidCandIndex)
					{
					break;
					}
				candidates++;
				}
			if (gZoneUtilArray[nextBestIndex].iPhysPages == gZoneUtilArray[nextBestIndex].iAllocFixed + 
															gZoneUtilArray[nextBestIndex].iAllocUnknown ||
				candidates != 1)
				{
				test.Printf(_L("Cannot find zone or too many equal pref zones to perform test - Skipping...\n"));
				goto SkipTest5;
				}
			}



		for (TUint i = 0; i < gZoneCount; i++)
			{
			if (i != (TUint)best)
				{
				freeInOtherZones += gZoneUtilArray[i].iFreePages;
				}
			}
		allocatablePages = 	gZoneUtilArray[best].iFreePages + 
							gZoneUtilArray[best].iAllocMovable + 
							gZoneUtilArray[best].iAllocDiscardable;
		
		if (allocatablePages > freeInOtherZones)
			{
			test.Printf(_L("Not enough free RAM for test - Skipping...\n"));
			goto SkipTest5;
			}
		
		// Allocate the fixed array before getting any page counts
		r = Ldd.AllocFixedArray(allocatablePages + 1);
		if (r != KErrNone)
			{
			test.Printf(_L("Failed to allocate fixed array r = %d - Skipping...\n"), r);
			goto SkipTest5;
			}
		
		// Fill the RAM zone with movable pages if none already in it.
		GetAllPageInfo();
		if (gZoneUtilArray[best].iAllocMovable == 0)
			{
			if (!gZoneUtilArray[best].iFreePages)
				{
				test.Printf(_L("RAM zone ID %x too full for test - Skipping...\n"), gZoneConfigArray[best].iZoneId);
				goto SkipTest5;
				}
			// Fill the zone with movable pages
			r = ZoneAllocMovable(gChunkArray1, gChunkArraySize1, best);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to fill zone index %d with movable r = %d\n"), best, r);
				CLEANUP(ResetRamZoneFlags());
				TEST_FAIL;
				}
			
			if (GetBestZone(BEST_FIXED) != best)
				{
				test.Printf(_L("Unable to complete test; RAM zone reordering - Skipping...\n"));
				goto SkipTest5;
				}
			}	
		// Allocate fixed pages after reseting the allocation flags.
		GetAllPageInfo();
		ResetRamZoneFlags();
		allocatablePages = 	gZoneUtilArray[best].iFreePages + 
							gZoneUtilArray[best].iAllocMovable + 
							gZoneUtilArray[best].iAllocDiscardable;
		switch (testIndex)
			{
			case 0:
				r = Ldd.AllocateFixed2(allocatablePages);
				GetAllPageInfo();
				allocImmovPages = gZoneUtilArray[best].iAllocFixed + gZoneUtilArray[best].iAllocUnknown;
				if (r != KErrNone || gZoneConfigArray[best].iPhysPages != allocImmovPages)
					{
					test.Printf(_L("RAM zone ID 0x%x not full of immovable pages\n"), gZoneConfigArray[best].iZoneId);
					CLEANUP(ResetRamZoneFlags());
					TEST_FAIL;
					}
				else
					{
					test.Printf(_L("Pass: Pages allocated to the zone expected\n"));
					}
				break;

			case 1:
				GetOriginalPageCount();
				r = Ldd.AllocateFixed2(allocatablePages + 1);
				GetAllPageInfo();
				allocImmovPages = gZoneUtilArray[best].iAllocFixed + gZoneUtilArray[best].iAllocUnknown;
				if (r != KErrNone || gZoneUtilArray[best].iPhysPages != allocImmovPages ||
					gZoneUtilArray[nextBestIndex].iAllocFixed <= gOriginalPageCountArray[nextBestIndex].iAllocFixed)
					{
					test.Printf(_L("RAM zone ID 0x%x not full of immovable pages or next best ID 0x%x no fix pages\n"), 
									gZoneConfigArray[best].iZoneId, gZoneConfigArray[nextBestIndex].iZoneId);
					test.Printf(_L("nextBest %d origFix 0x%x curFix 0x%x\n"), 
									nextBestIndex, gOriginalPageCountArray[nextBestIndex].iAllocFixed, 
									gZoneUtilArray[nextBestIndex].iAllocFixed);
					CLEANUP(ResetRamZoneFlags());
					TEST_FAIL;
					}
				// Go through every other zone and check that fixed pages haven't increased
				for (TUint i = 0; i < gZoneCount; i++)
					{
					if (i != (TUint)best && i != (TUint)nextBestIndex &&
						gZoneUtilArray[i].iAllocFixed > gOriginalPageCountArray[i].iAllocFixed)
						{
						test.Printf(_L("FAIL: Fix page count increased zoneIndex %d orig 0x%x current 0x%x\n"), 
										i, gOriginalPageCountArray[i].iAllocFixed, gZoneUtilArray[i].iAllocFixed);
						CLEANUP(ResetRamZoneFlags());
						TEST_FAIL;
						}
					}

				test.Printf(_L("Pass: Pages allocated to the zone expected\n"));
				break;
			}
	SkipTest5 :
		// This will perform any required clean up.
		ResetRamZoneFlags();
		TestEnd();
	}

	test.Next(_L("Test7: Check which zone a discardable page has been allocated to"));	
	TestStart();
	if (gPagedRom)
		{
		GetAllPageInfo();

		// Try to allocate just one more discardable page
		TUint allocBytes = (gTotalPageCount.iDiscardablePages + 1) << gPageShift;
		TInt discardablePages;

		best = GetBestZone(BEST_DISCARDABLE);
		if (best == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping...\n"));
			}
		else
			{
			// Populate the candidate list
			GetCandList2(best);
			zoneFound = EFalse;
			GetOriginalPageCount();
			TInt r = AllocDiscardable(discardablePages, allocBytes);
			if (r != KErrNone)
				{
				test.Printf(_L("Fail: r %d\n"), r);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}

			r = VerifyMovDisAlloc();
			// Need to check all candidates to see if page has gone into any one of them
			for (TUint i=0; i < gZoneCount; i++)
				{
				if (gCandList2[i] == KInvalidCandIndex)
					{
					break;
					}
				TUint zoneIndex = gCandList2[i];
				if (gOriginalPageCountArray[zoneIndex].iAllocDiscardable < gZoneUtilArray[zoneIndex].iAllocDiscardable)
					{
					zoneFound = ETrue;
					break;
					}
				}

			if (r == KErrNone && zoneFound)
				{
				test.Printf(_L("Pass: Discardable allocated to the zone expected\n"));
				}
			else
				{
				test.Printf(_L("Fail: Discardable been allocated to a zone that was not expected r %d\n"), r);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}	
			}
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();

	test.Next(_L("Test8: Check fixed page allocations (contiguous) will move pages"));
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	
	TInt r = KErrNone;
	TUint testZoneIndex = 0;

	// Get the most pref zone which is completely free to use as a test zone
	r = FindMostPrefEmpty(testZoneIndex);
	if (r != KErrNone)
		{
		test.Printf(_L("Cannot find empty zone - Skipping...\n"));
		goto skipTest8;
		}

		
	// fill the test zone with movable pages
	r = ZoneAllocMovable(gChunkArray1, gChunkArraySize1, testZoneIndex);
	GetAllPageInfo();
	if (r != KErrNone ||
		gZoneUtilArray[testZoneIndex].iAllocMovable != gZoneUtilArray[testZoneIndex].iPhysPages)
		{
		test.Printf(_L("Failed to allocate movable r = %d - Skipping...\n"), r);
		goto skipTest8;
		}

	if (gTotalPageCount.iFreePages < 1)
		{
		test.Printf(_L("Insufficient memory - totalFreePages = 0x%x - Skipping...\n"), gTotalPageCount.iFreePages);
		goto skipTest8;
		}

	for (TUint zoneIndex = 0; zoneIndex < gZoneCount; zoneIndex++)
		{
		TUint zoneId = gZoneConfigArray[zoneIndex].iZoneId;
		if (zoneIndex != testZoneIndex)
			{
			r = Ldd.SetZoneFlag(zoneId, gZoneConfigArray[zoneIndex].iFlags, NO_FIXED_FLAG);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to set flag on zone index %d r = %d- Skipping...\n"), zoneIndex, r);
				goto skipTest8;
				}
			}
		}
	
	//attempt to alloc 1 contiguous fixed page
	r = Ldd.AllocContiguous(gPageSize);
	GetAllPageInfo();
	if (r != KErrNone ||
		!gZoneUtilArray[testZoneIndex].iAllocFixed)
		{
		test.Printf(_L("FAIL: no fixed pages in testZoneIndex(%d) r = %d\n"), testZoneIndex, r);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));
		}
	
skipTest8:
	TestEnd();

	test.Next(_L("Test9: Allocate a large 1MB aligned chunk and touch all its pages"));
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;

	GetAllPageInfo();
	const TUint KChunkBytes = 0x100000;
	const TUint KChunkMask = KChunkBytes - 1;
	TUint freeBytes = gTotalPageCount.iFreePages << gPageShift;
	TUint chunkSize = freeBytes & ~KChunkMask;
	// Fill as much memory as possible with fixed pages while still allowing the movable allocation to succeed.
	// This should help force the page table allocation for the chunk to need to move pages.
	TUint fixedSize = freeBytes - chunkSize;
	r = KErrNoMemory;
	while (r != KErrNone && fixedSize)
		{
		Ldd.FreeAllFixedPages();
		fixedSize -= gPageSize;
		test.Printf(_L("fixedSize 0x%x\n"), fixedSize);
		r = Ldd.AllocateFixed(fixedSize >> gPageShift);
		if (r != KErrNone)
			break;
		r = AllocMovable(gChunkArray1, gChunkArraySize1, gChunkArraySize1, chunkSize);
		}
	if (r == KErrNone)
		{
		// Touch every page in the chunk.
		memclr(gChunkArray1->Base(), chunkSize);
		}
	TestEnd();	// This will free any chunk and fixed pages.

	test.End();
	return KErrNone;
	}

	
//
// TestMovingPages
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0526
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying that pages are moved correctly
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Write to all chunks apart from chunk 9. 
//! 		Following this start a RAM defrag and whilst defrag is running write to chunk 9. 
//! 	2.	Fragment the memory. Start a RAM defrag and whilst defrag is running 
//! 		continuously write different values to the chunks. 
//! 	3.	Fragment the memory. Following this start a RAM defrag and whilst this 
//! 		is happening, continuously read from all chunks. 
//! 	4.	Allocate some movable pages. Call a device driver that will allocate fixed 
//! 		pages and write values to these fixed pages. 
//! 		Close every other chunk so that only 5 chunks are now still open. 
//! 		Following this perform a RAM defrag. 
//! 		Read from the fixed pages that were originally written to, ensuring 
//! 		that they have not changed.   
//! 	5.	Without starting any processes, allocate discardable pages by loading 
//! 		pages that are demand paged. 
//! 		Read each of the constants from beginning to end. 
//! 		Following this perform a RAM defrag. 
//! 
//! @SYMTestExpectedResults
//! 	1.	Zones are emptied
//! 	2.	Zones are emptied	
//! 	3.	Zones are emptied
//! 	4.	The values written to the fixed pages have not changed. 
//! 	5.	Zones are emptied
//---------------------------------------------------------------------------------------------------------------------
TInt TestMovingPages()
	{
	const TInt KAllChunks = -1;  // Specifies that all chunks should be written to
	
	test.Start(_L("Test1: Whilst moving page, change the usage "));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks, KChunkDefaultSize);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	// Find a chunk that exists
	TInt chunkIndex = gChunkArraySize1 - 1;
	for (; chunkIndex >= 0 && gChunkArray1[chunkIndex].Handle() == NULL; chunkIndex--);
	if (chunkIndex < 0)
		{
		test.Printf(_L("No chunks were allocated\n"));
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}

	WriteToChunk(gChunkArray1, gChunkArraySize1, chunkIndex);

	TInt r = gTestThread.Create(gTestThreadName,MultiGenDefragThreadFunc,KDefaultStackSize,0x1000,0x1000,NULL);
	if (r != KErrNone)
		{
		test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}	
	gTestThread.Logon(status);
	gTestThread.Resume();	

	TUint8* base = gChunkArray1[chunkIndex].Base();
	while (status.Int() == KRequestPending)
		{
		User::After(10000);
		for (TInt8 k = 0; k < 10; k ++)
			{
			if (base == gChunkArray1[chunkIndex].Base() + gChunkArray1[chunkIndex].Size())
				{
				base = gChunkArray1[chunkIndex].Base();
				}
			*base++ = k; // write 0 - 9 to the chunk
			}
		}
	
	User::WaitForRequest(status);
	r = status.Int();
	TESTDEBUG(test.Printf(_L("defrag running on another thread returns %d\n"), r));

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);

	if (r == KErrNone)
		{
		test.Printf(_L("Correct return value\n"));
		test.Printf(_L("Passed...\n"));
		}
	else
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrNone);
		TEST_FAIL;	
		}
		
	gTestThread.Close();
	TestEnd();
	
	
	test.Next(_L("Test2: Whilst moving page, change the contents of the page"));	
	TestStart();	
		
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks, KChunkDefaultSize);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	WriteToChunk(gChunkArray1, gChunkArraySize1, KAllChunks);
	
	r = gTestThread.Create(gTestThreadName,MultiGenDefragThreadFunc,KDefaultStackSize,0x1000,0x1000,NULL);
	if (r != KErrNone)
		{
		test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}	
	gTestThread.Logon(status);
	gTestThread.Resume();

	TUint8 startValue = 0;
	while (status.Int() == KRequestPending)
		{
		User::After(10000);
		WriteToChunk(gChunkArray1, gChunkArraySize1, KAllChunks, startValue);
		if (++startValue > 245)
			{
			startValue = 0;
			}
		}
		
	User::WaitForRequest(status);
	r = status.Int();
	TESTDEBUG(test.Printf(_L("defrag running on another thread returns %d\n"), r));
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);

	if (r == KErrNone)
		{
		test.Printf(_L("Correct return value\n"));
		test.Printf(_L("Passed...\n"));	
		}
	else
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrNone);
		TEST_FAIL;	
		}

	gTestThread.Close();
	TestEnd();
	

	test.Next(_L("Test3: Whilst moving page, read pages"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks, KChunkDefaultSize);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	WriteToChunk(gChunkArray1, gChunkArraySize1, KAllChunks);
	
	r = gTestThread.Create(gTestThreadName,MultiGenDefragThreadFunc,KDefaultStackSize,0x1000,0x1000,NULL);
	if (r != KErrNone)
		{
		test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}	
	gTestThread.Logon(status);
	gTestThread.Resume();

	while (status.Int() == KRequestPending)
		{
		User::After(100000000);
		ReadChunk(gChunkArray1, gChunkArraySize1);
		}		
	
	User::WaitForRequest(status);
	r = status.Int();
	TESTDEBUG(test.Printf(_L("defrag running on another thread returns %d\n"), r));

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);

	if (r == KErrNone)
		{
		test.Printf(_L("Correct return value\n"));
		test.Printf(_L("Passed...\n"));	
		}
	else
		{
		test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
		TEST_FAIL;	
		}
		
	gTestThread.Close();
	TestEnd();

	test.Next(_L("Test4: Allocate fixed pages and then perform a defrag"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	Ldd.AllocateFixedWrite(FILL_ALL_FIXED);

	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	test.Printf(_L("defrag returns %d\n"), r);

	if (CheckZonesSwitchedOff() != EFalse)
		{
		test.Printf(_L("Fail: Zones were switched off when they shouldn't have been\n"));
		CLEANUP(r = Ldd.FreeAllFixedPagesRead());
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	r = Ldd.FreeAllFixedPagesRead();
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);	
	TestEnd();
	
	test.Next(_L("Test5: Allocate discardable pages and then perform a defrag"));	
	TestStart();	
	if (gPagedRom)
		{
		TInt discardablePages;
				
		UpdateRamInfo();
		r = AllocDiscardable(discardablePages);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			TEST_FAIL;
			}
		TESTDEBUG(test.Printf(_L("Number of discardable pages = 0x%x\n"), discardablePages >> gPageShift));

		GetOriginalPageCount();
		TBool genSucceed = CanGenSucceed();
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);

		TESTDEBUG(test.Printf(_L("defrag returns %d\n"), r));
		
		if (genSucceed && CheckZonesSwitchedOff() == EFalse)
			{
			test.Printf(_L("Fail: Zones were not switched off when they should have been\n"));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		ResetDPCache();
		
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();
		
	test.End();
	return 0;
	}
		

//
// TestMovPgsDefrag
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0527
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the moving of pages in the defrag implmentation
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Following this perform a RAM defrag. 
//! 		Now that all movable memory has now been moved and the relevant source
//! 		zones have been switched off, again attempt to perform a RAM defrag. 
//! 	2.	Fragment the memory. Following this perform a RAM defrag and whilst 
//! 		this is happening, continuously create more chunks. 
//!		3.	Set up memory so that there are a mix of movable and free pages in a low preference zone, with there
//!			being movable pages in the higher preference zones. Following this call a general defrag
//!		4.	Set up memory so that there are a mix of movable and free pages and a couple of free pages 
//!			in a low preference zone, with there being enough free pages in the higher preference zones for the 
//!			pages to be moved to. Following this call a general defrag
//!		5.	Set up memory so that there is a fixed page and a movable page in an empty zone (testZone). Also ensure that 
//!			there are movable and free pages in more preferable zones than the test zone. Following this call a general
//!			defrag
//!		6.	Set up memory so that the mostPrefZone contains movable and free pages. Allocate 1 movable and 1 fixed
//!			page into an empty zone (testZone). Following this call a general defrag. 
//! 
//! @SYMTestExpectedResults
//! 	1.	Second defrag does not empty any zones
//! 	2.	No zones have been emptied
//!		3.	Zones are emptied if the general defrag can succeed
//! 	4.	Defrag fills up the less preferable zones first
//!		5.	movable pages are moved from the testZone
//!		6.	testZone is not emptied
//---------------------------------------------------------------------------------------------------------------------
TInt TestMovPgsDefrag()
	{
	TInt r = KErrNone;

	test.Start(_L("Test1: Performing a defrag twice"));
	TestStart();
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	TESTDEBUG(test.Printf(_L("After calling general defrag r = %d\n"), r));

	r = VerifyMovDisAlloc();	
	// The first defrag should empty zones if the general can succeed 
	if (r != KErrNone)
		{
		test.Printf(_L("Fail: r = %d, memory is not laid out as expected\n"), r);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}

	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	
	TESTDEBUG(test.Printf(_L("After calling general defrag again r = %d\n"), r));
	
	// The second call to general defrag should have nothing further to do
	if (r != KErrNone || CheckZonesSwitchedOff() != EFalse)
		{
		test.Printf(_L("Fail: r = %d, expected = %d, or zones have been switched off \n"), 
						r, KErrNone);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);	
	TestEnd();	
	
	
	
	test.Next(_L("Test2: Ensure new memory allocations can occur successfully during a general defrag"));
	TestStart();
	
	// Create a thread to call Defrag continuously and resume it
	r = gTestThread.Create(gTestThreadName,MultiLoopGenDefragThreadFunc,KDefaultStackSize,0x1000,0x1000,NULL);
	if (r != KErrNone)
		{
		test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
		TEST_FAIL;
		}		
	gTestThread.Logon(status);
	gTestLoop = ETrue;
	gTestThread.Resume();
	
	// Whilst the defrag loop is taking place, continuously allocate and free memory
	for (TInt i = 0; i < 100; i++)
		{
		r = AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
		if (r != KErrNone)
			break;
		RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
		}
	gTestLoop = EFalse;
	User::WaitForRequest(status);
	TESTDEBUG(test.Printf(_L("defrag running on another thread returns %d\n"), r));
		
	if (r != KErrNone || status.Int() != KErrNone)
		{
		test.Printf(_L("Fail: r = %d, status.Int() = %d\n"), r, status.Int());	
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	RemoveChunkAlloc(gChunkArray2, gChunkArraySize2);
	gTestThread.Close();	
	TestEnd();
	
	
	test.Next(_L("Test3: Check whether RAM defrag switches off any zones"));
	TestStart();
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetOriginalPageCount();
	TBool genSucceed = CanGenSucceed();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	
	TESTDEBUG(test.Printf(_L("After calling zone defrag r = %d\n"), r));
	
	if (r != KErrNone || (genSucceed && CheckZonesSwitchedOff() == EFalse))
		{
		test.Printf(_L("Fail: No zones were switched off or r = %d, expected = %d\n"), r, KErrNone);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test4: Verify that a general defrag moves pages to the least preferable RAM zone in use"));
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	// Get the preference order
	GetPrefOrder();

	TUint mostPrefIndex = 0;
	TUint mostPrefArrayIndex = 0;
	TUint totMov = 0;
	TUint lastPrefIndex = 0;
	TUint lastPrefArrayIndex = 0;
	TUint movPages = 0;
	TUint movBytes = 0;
	TUint totFreeInTestZones = 0;
	TBool zoneNotEmptyOrFull = EFalse;


	// Find the first most pref zone that has free pages in it
	r = FindMostPrefWithFree(mostPrefIndex, &mostPrefArrayIndex);
	if (r != KErrNone)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping...\n"));
		goto skipTest4;
		}

	// Find the next most preferable free zone
	lastPrefArrayIndex = mostPrefArrayIndex + 1;

	while (lastPrefArrayIndex < gZoneCount &&
		gZoneUtilArray[gPrefArray[lastPrefArrayIndex]].iFreePages != gZoneUtilArray[gPrefArray[lastPrefArrayIndex]].iPhysPages)
		{
		lastPrefArrayIndex++;
		}

	if (lastPrefArrayIndex >= gZoneCount)
		{
		test.Printf(_L("Skipping...\n"));
		goto skipTest4;
		}

	// Block all other zones
	for (TUint prefIndex = lastPrefArrayIndex + 1; prefIndex < gZoneCount; prefIndex++)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		Ldd.SetZoneFlag(gZoneConfigArray[zoneIndex].iZoneId, gZoneConfigArray[zoneIndex].iFlags, NO_ALLOC_FLAG);
		}


	// Zone alloc to fill half of the last zone under test
	GetAllPageInfo();
	lastPrefIndex = gPrefArray[lastPrefArrayIndex];
	r = Ldd.ZoneAllocDiscontiguous(gZoneConfigArray[lastPrefIndex].iZoneId, gZoneUtilArray[lastPrefIndex].iFreePages >> 1);
	if (r != KErrNone)
		{
		test.Printf(_L("Failed to allocate fixed pages r = %d - Skipping...\n"), r);
		goto skipTest4;
		}


	// Go through the zones under test and determine the number of free pages
	GetAllPageInfo();
	for(TUint prefIndex = mostPrefArrayIndex; prefIndex <= lastPrefArrayIndex; prefIndex++)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		totFreeInTestZones += gZoneUtilArray[zoneIndex].iFreePages;
		}

	// Allocate movable pages to almost fill zones under test
	movPages = totFreeInTestZones;
	movBytes = movPages << gPageShift;
	while (movBytes && AllocMovable(gChunkArray1, gChunkArraySize1, 1, movBytes) != KErrNone)
		{
		movBytes -= gPageSize;
		movPages--;
		}
	if (!movBytes)
		{
		test.Printf(_L("Failed to allocate 0x%x movable pages - Skipping...\n"), movPages);
		goto skipTest4;
		}


	// Free the allocated fixed pages
	Ldd.FreeAllFixedPages();

	// Reset all the flags
	ResetRamZoneFlags();
	
	// Check that the most preferable zone still has movable pages in it
	// and also check that the last zone under test has free pages in it
	GetAllPageInfo();
	if (gTotalPageCount.iMovablePages < movPages ||
		gZoneUtilArray[mostPrefIndex].iAllocMovable == 0 ||
		gZoneUtilArray[lastPrefIndex].iFreePages == 0)
		{
		test.Printf(_L("Setup failed - Skipping...\n"));
		goto skipTest4;
		}

	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	// Check that defrag allocated the movable from the less preferable to the more preferable
	GetPrefOrder();
	totMov = gTotalPageCount.iMovablePages;
	for(TInt prefIndex = gZoneCount - 1; prefIndex >= 0 ; prefIndex--)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		GetCandList1(zoneIndex);
		TUint candIndex = 0;
		for (; candIndex < gZoneCount; candIndex++)
			{// Check all the zones of this preference
			TInt zoneIndexCand = gCandList1[candIndex];
			if (zoneIndexCand == KInvalidCandIndex)
				{
				break;
				}
			totMov -= gZoneUtilArray[zoneIndexCand].iAllocMovable;
			if (gZoneUtilArray[zoneIndexCand].iFreePages &&
				(gZoneUtilArray[zoneIndexCand].iAllocMovable || 
				gZoneUtilArray[zoneIndexCand].iAllocDiscardable))
				{
				zoneNotEmptyOrFull = ETrue;
				}
			}
		prefIndex -= candIndex - 1 ;

		if (zoneNotEmptyOrFull && totMov != 0)
			{
			test.Printf(_L("FAIL: index = %d free = 0x%x totMov = 0x%x\n"), 
							zoneIndex, gZoneUtilArray[zoneIndex].iFreePages, totMov);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
			TEST_FAIL;
			}
		}

skipTest4:
	TestEnd();


	test.Next(_L("Test5: Verify that a general defrag tidies up if RAM zone to be emptied contains fixed pages"));
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	gChunkArray2 = new RChunk;
	gChunkArraySize2 = 1;

	// find 2 empty zones - put 1 movable in 1 of them and 1 movable and 1 fixed in the other
	TUint testZoneIndex1;
	TUint testZoneIndex2;
	TUint testZoneID2;
	TUint numFreeInUse = 0;

	r = FindMostPrefEmpty(testZoneIndex1);
	if (r != KErrNone)
		{
		test.Printf(_L("Skipping...\n"));
		goto skipTest5;
		}
	// Allocate 1 movable page to the test zone
	r = ZoneAllocMovable(gChunkArray1, gChunkArraySize1, testZoneIndex1, 1);
	if (r != KErrNone)
		{
		test.Printf(_L("Failed to allocate 1 movable page to zone index %d, r = %d - Skipping...\n"), testZoneIndex1, r);
		goto skipTest5;
		}
	
	r = FindMostPrefEmpty(testZoneIndex2);
	if (r != KErrNone)
		{
		test.Printf(_L("Skipping...\n"));
		goto skipTest5;
		}
	// Allocate 1 movable page to the test zone
	r = ZoneAllocMovable(gChunkArray2, gChunkArraySize2, testZoneIndex2, 1);
	if (r != KErrNone)
		{
		test.Printf(_L("Failed to allocate 1 movable page to zone index %d, r = %d - Skipping...\n"), testZoneIndex2, r);
		goto skipTest5;
		}

	// Zone alloc to put 1 fixed page last zone under test
	testZoneID2 = gZoneConfigArray[testZoneIndex2].iZoneId;
	r = Ldd.ZoneAllocContiguous(testZoneID2, gPageSize);
	if (r != KErrNone)
		{
		test.Printf(_L("Failed to allocate 1 fixed page to zone %d index, r = %d - Skipping...\n"), testZoneIndex2, r);
		goto skipTest5;
		}
	
	// Allocate 2 fixed (as there is 1 in our test zone) to prevent reordering
	GetAllPageInfo();
	for (TUint index = 0; index < gZoneCount; index++)
		{
		TUint totImmovable = gZoneUtilArray[index].iAllocFixed + gZoneUtilArray[index].iAllocUnknown ;
		if (index != testZoneIndex1 &&
			index != testZoneIndex2 &&
			gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages &&
			totImmovable < 2)
			{
			r = Ldd.ZoneAllocToMany(index, 2 - totImmovable);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to alloc 0x%x fixed to zone index %d r = %d - Skipping...\n"), 
								2, index, r);
				goto skipTest5; 
				}
			}
		}

	// Check that the number of free pages in the other in-use zones >= movable pages in test zone
	GetAllPageInfo();
	for (TUint index = 0; index < gZoneCount; index++)
		{
		if (index != testZoneIndex1 &&
			index != testZoneIndex2 &&
			gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages)
			{
			numFreeInUse += gZoneUtilArray[index].iFreePages;
			}
		}
	if (gZoneUtilArray[testZoneIndex1].iAllocMovable + gZoneUtilArray[testZoneIndex1].iAllocDiscardable + 
		gZoneUtilArray[testZoneIndex2].iAllocMovable + gZoneUtilArray[testZoneIndex2].iAllocDiscardable > numFreeInUse ||
		gZoneUtilArray[testZoneIndex2].iAllocFixed == 0)
		{
		test.Printf(_L("Setup failed - Skipping...\n"));
		goto skipTest5;
		}

	// Added extra tracing to debug random failures
	PrintPageInfo();

	// Call a general defrag
	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);

	// Check that the defrag tidies up
	GetAllPageInfo();
	if(gZoneUtilArray[testZoneIndex1].iAllocMovable ||
		gZoneUtilArray[testZoneIndex2].iAllocMovable)
		{
		test.Printf(_L("FAIL: testZoneIndex1(%d): mov orig 0x%x cur 0x%x, testZoneIndex2(%d) mov orig 0x%x cur 0x%x \n"), 
						testZoneIndex1, gOriginalPageCountArray[testZoneIndex1].iAllocMovable, gZoneUtilArray[testZoneIndex1].iAllocMovable, 
						testZoneIndex2, gOriginalPageCountArray[testZoneIndex2].iAllocMovable, gZoneUtilArray[testZoneIndex2].iAllocMovable);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}

skipTest5:
	TestEnd();


	test.Next(_L("Test6: Verify that a general defrag will not move pages if RAM zone to be emptied contains fixed pages"));
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	// Find the first most pref zone that has free pages in it
	r = FindMostPrefWithFree(mostPrefIndex, &mostPrefArrayIndex);
	if (r != KErrNone)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping...\n"));
		goto skipTest6;
		}

	// Find the next most preferable free zone
	lastPrefArrayIndex = mostPrefArrayIndex + 1;
	while (lastPrefArrayIndex < gZoneCount &&
		gZoneUtilArray[gPrefArray[lastPrefArrayIndex]].iFreePages != gZoneUtilArray[gPrefArray[lastPrefArrayIndex]].iPhysPages)
		{
		lastPrefArrayIndex++;
		}
	if (lastPrefArrayIndex >= gZoneCount)
		{
		test.Printf(_L("Skipping...\n"));
		goto skipTest6;
		}


	// Zone alloc to put 1 fixed page last zone under test
	lastPrefIndex = gPrefArray[lastPrefArrayIndex];
	r = Ldd.ZoneAllocDiscontiguous(gZoneConfigArray[lastPrefIndex].iZoneId, 1);
	if (r != KErrNone)
		{
		test.Printf(_L("Failed to allocate 1 fixed page to zone index %d r = %d - Skipping...\n"), lastPrefIndex, r);
		goto skipTest6;
		}
	
	GetAllPageInfo();
	if (!gZoneUtilArray[mostPrefIndex].iFreePages)
		{
		test.Printf(_L("Not enough space in zone under test  mostPrefIndex = %d - Skipping...\n"), mostPrefIndex);
		goto skipTest6;
		}

	// Block all zones apart from the last zone under test from allocation
	for (TUint prefIndex = 0; prefIndex < gZoneCount; prefIndex++)
		{
		TUint zoneIndex = gPrefArray[prefIndex];
		if (zoneIndex != lastPrefIndex)
			{
			Ldd.SetZoneFlag(gZoneConfigArray[zoneIndex].iZoneId, gZoneConfigArray[zoneIndex].iFlags, NO_ALLOC_FLAG);
			}
		}
	
	TESTDEBUG(test.Printf(_L("mostPrefIndex = %d lastPrefIndex = %d\n"), mostPrefIndex, lastPrefIndex));
	
	// Allocate movable pages to the lastPrefZone that will fit into the most pref zone
	movPages = gZoneUtilArray[mostPrefIndex].iFreePages;
	movBytes = movPages << gPageShift;
	while (movBytes && AllocMovable(gChunkArray1, gChunkArraySize1, 1, movBytes) != KErrNone)
		{
		movBytes -= gPageSize;
		}
	if (!movBytes)
		{
		test.Printf(_L("Failed to allocate 0x%x movable pages r = %d - Skipping...\n"), movPages, r);
		goto skipTest6;
		}

	// Reset all the flags
	ResetRamZoneFlags();
	

	// Check that the number of movable pages in the least pref will fit
	// into the most preferable zone
	GetAllPageInfo();
	if (gZoneUtilArray[lastPrefIndex].iAllocMovable > gZoneUtilArray[mostPrefIndex].iFreePages)
		{
		test.Printf(_L("Setup failed - Skipping...\n"));
		PrintPageInfo();
		goto skipTest6;
		}
	
	// Added extra tracing to debug random failures
	PrintPageInfo();	

	// Call a general defrag
	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);

	// Check that the defrag didn't move any pages from the least pref zone
	GetAllPageInfo();
	if(gOriginalPageCountArray[lastPrefIndex].iAllocMovable > gZoneUtilArray[lastPrefIndex].iAllocMovable)
		{
		test.Printf(_L("FAIL: mostPref(index %d): origMov = 0x%x curMov = 0x%x, lastPref(index %d): origMov = 0x%x curMov = 0x%x \n"), 
						mostPrefIndex, gOriginalPageCountArray[mostPrefIndex].iAllocMovable, gZoneUtilArray[mostPrefIndex].iAllocMovable, 
						lastPrefIndex, 	gOriginalPageCountArray[lastPrefIndex].iAllocMovable, gZoneUtilArray[lastPrefIndex].iAllocMovable);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}

skipTest6:
	TestEnd();

	test.End();
	return KErrNone;
	}

//
// GenDefragTest123Setup
//
// Used to set up the memory ready for testing in TestGenDefrag, Test Steps 1, 2, 3 and 4
//
// @param aMovAlloc The number of movable pages to allocate.
// @param aFixAlloc The number of fixed pages to allocate.
// @param aDisAlloc The number of discardable pages to allocate.
// @param aMovIndex On return contains the test zone index.
// @param aNumFreeInUse On return contains the number of free pages in the inuse zones.
// @param aTestNo The test number that is using this setup.
//
TInt GenDefragTest123Setup(TUint aMovAlloc, TUint aFixAlloc, TUint aDisAlloc, TUint& aMovIndex, TUint& aNumFreeInUse, TUint aTestNo)
	{
	TInt r = KErrNone;
	aNumFreeInUse = 0;
	const TUint KFillUpTo = 5;	// Rough estimate of how full the in-use zones should be
	const TUint KSpaceNeeded = 2;
	TUint movPrefIndex = 0;
	if (aTestNo == 4)
		{// Test 4 we need to look for the LEAST preferable RAM zone which is empty
		TUint mostPrefIndex;
		r = FindLeastPrefEmpty(aMovIndex, &movPrefIndex);
		TInt r2 = FindMostPrefEmpty(mostPrefIndex);
		if (r != KErrNone || r2 != KErrNone || !gPagedRom ||
			gZoneConfigArray[aMovIndex].iPref == gZoneConfigArray[mostPrefIndex].iPref)
			{
			test.Printf(_L("r %d r2 %d or not a paged ROM or Equal zone preferences- Skipping test step...\n"), 
							r, r2);
			return KErrGeneral;
			}
		}
	else
		{// All other tests look the MOST preferable RAM zone which is empty
		r = FindMostPrefEmpty(aMovIndex, &movPrefIndex);
		if (r != KErrNone || !gPagedRom)
			{
			test.Printf(_L("No suitable RAM zone found or not a paged ROM - Skipping test step...\n"));
			return KErrGeneral;
			}
		}

	// Go through all the in-use zones and check how many free pages there are
	// Allocate fixed so there is only KFillUpTo pages in each in-use zone to
	// ensure that the total number of free pages in the in-use zones can fit into
	// the zone under test
	GetAllPageInfo();
	for (TUint index = 0; index < gZoneCount; index++)
		{
		if (gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages &&
			gZoneUtilArray[index].iFreePages > KFillUpTo)
			{
			r = Ldd.ZoneAllocToMany(index, gZoneUtilArray[index].iFreePages - KFillUpTo);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to alloc 0x%x fixed to zone index %d r = %d - Ending setup...\n"), 
								gZoneUtilArray[index].iFreePages - KFillUpTo, index, r);
				return KErrGeneral; 
				}
			}
		}

	GetAllPageInfo();
	// If any of the in-use zones doesn't contain any fixed and unknown pages allocate one fixed
	for (TUint index = 0; index < gZoneCount; index++)
		{
		if (gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages &&
			!gZoneUtilArray[index].iAllocFixed && !gZoneUtilArray[index].iAllocUnknown)
			{
			r = Ldd.ZoneAllocToMany(index, 1);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to alloc 0x%x fixed to zone index %d r = %d - Ending setup...\n"), 
								1, index, r);
				return KErrGeneral;
				}
			}
		}

	GetAllPageInfo();
	// Check the total number of free pages in the in-use zones
	for (TUint index = 0; index < gZoneCount; index++)
		{
		if (gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages)
			{
			aNumFreeInUse += gZoneUtilArray[index].iFreePages;
			}
		}
	TESTDEBUG(test.Printf(_L("aNumFreeInUse = 0x%x\n"), aNumFreeInUse));

	test.Printf(_L("aMovIndex = %d\n"), aMovIndex);
	if (aNumFreeInUse < KSpaceNeeded)
		{
		test.Printf(_L("RAM zone to be tested is too full - Skipping test step...\n"));
		return KErrGeneral;
		}


	// Ensure that there are movable pages in the zone under test
	// ensuring that there is also room for discardable pages
	GetAllPageInfo();
	if (aMovAlloc != 0 && gZoneUtilArray[aMovIndex].iAllocMovable == 0)
		{
		// Allocate just aMovAlloc movable pages
		TUint movPagesAlloc = aMovAlloc;
		if (aTestNo == 2 || aTestNo == 4)
			{// Tests 2 and 4 require that movable pages can't be moved from zone
			movPagesAlloc = aNumFreeInUse + 1;
			if (movPagesAlloc > gZoneUtilArray[aMovIndex].iFreePages)
				{
				test.Printf(_L("Insufficiant space in zone %d to allocate 0x%x pages - Ending setup...\n"), 
								aMovIndex, movPagesAlloc);
				return KErrGeneral;
				}
			}

		r = ZoneAllocMovable(gChunkArray1, gChunkArraySize1, aMovIndex, movPagesAlloc);
		if (gZoneUtilArray[aMovIndex].iAllocMovable != movPagesAlloc ||
			gZoneUtilArray[aMovIndex].iFreePages == 0 ||
			r != KErrNone)
			{// Movable page didn't go into RAM, or RAM zone is full.
			test.Printf(_L("Can't complete test with current RAM layout. - Skipping...\n"));
			test.Printf(_L("zone mov 0x%x free 0x%x r=%d\n"), 
						gZoneUtilArray[aMovIndex].iAllocMovable,	
						gZoneUtilArray[aMovIndex].iFreePages, r);
			return KErrGeneral;
			}
		}

	// Block all other zones from allocation
	for(TUint i = 0; i < gZoneCount; i++)
		{
		if (i != aMovIndex)
			{
			r = Ldd.SetZoneFlag(gZoneConfigArray[i].iZoneId, gZoneConfigArray[i].iFlags, NO_ALLOC_FLAG);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to set flag on zone index %d r = %d- Ending setup...\n"), i, r);
				return KErrGeneral;
				}
			}
		}

	// Now ensure that there are fixed pages in the zone under test
	// ensuring that there is also room for discardable pages
	GetAllPageInfo();
	if (aFixAlloc != 0 && gZoneUtilArray[aMovIndex].iAllocFixed == 0)
		{//Allocate just aFixAlloc fixed pages 
		r = Ldd.ZoneAllocToMany(aMovIndex, aFixAlloc);
		GetAllPageInfo();
		if (gZoneUtilArray[aMovIndex].iAllocMovable == 0 ||
			gZoneUtilArray[aMovIndex].iFreePages == 0 ||
			gZoneUtilArray[aMovIndex].iAllocFixed == 0 ||
			r != KErrNone)
			{// Fixed page didn't go into RAM or RAM zone is full.
			test.Printf(_L("Can't complete test with current RAM layout. - Skipping...\n"));
			test.Printf(_L("zone mov 0x%x fixed 0x%x free 0x%x r=%d\n"), 
						gZoneUtilArray[aMovIndex].iAllocMovable,	
						gZoneUtilArray[aMovIndex].iAllocFixed,	
						gZoneUtilArray[aMovIndex].iFreePages, r);
			return KErrGeneral;
			}
		}


	
	// Allocate aDisAlloc number of discardable pages
	if (aDisAlloc != 0 && gZoneUtilArray[aMovIndex].iAllocDiscardable == 0)
		{//Allocate just aDisAlloc discardable pages 
		TInt discard;
		r = ZoneAllocDiscard(aMovIndex, aDisAlloc, discard);
		GetAllPageInfo();
		if (r != KErrNone || gZoneUtilArray[aMovIndex].iAllocDiscardable == 0)
			{// Discardable page didn't go into RAM or RAM zone is full.
			test.Printf(_L("Can't complete test with current RAM layout. - Skipping...\n"));
			test.Printf(_L("zone mov 0x%x discardable 0x%x free 0x%x r=%d\n"), 
						gZoneUtilArray[aMovIndex].iAllocDiscardable,	
						gZoneUtilArray[aMovIndex].iAllocFixed,	
						gZoneUtilArray[aMovIndex].iFreePages, r);
			return KErrGeneral;
			}
		}
	
	// Update the total number of free pages in the in-use zones
	aNumFreeInUse = 0;
	for (TUint index = 0; index < gZoneCount; index++)
		{
		if (gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages &&
			index != aMovIndex)
			{
			aNumFreeInUse += gZoneUtilArray[index].iFreePages;
			}
		}
	TESTDEBUG(test.Printf(_L("aNumFreeInUse = 0x%x\n"), aNumFreeInUse));

	// now reset all the flags 
	ResetRamZoneFlags();
	
	return KErrNone;
	}
//
// TestGenDefrag
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0528
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying general scenarios when RAM defrag would take place
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Allocate 1 movable, 1 fixed and 1 discardable pages into the most preferable empty RAM zone
//!			ensuring that there is enough free space in the more preferable RAM zones for the
//!			movable pages in the RAM zone under test. Following this call a general defrag.
//!		2.	Allocate 1 movable and 1 discardable page into into the most preferable empty RAM zone
//!			ensuring that there is not enough free space in the more preferable RAM zones for the
//!			movable pages in the RAM zone under test. Following this call a general defrag. 
//!		3.	Allocate 1 movable and 1 discardable and 1 fixed page into the most preferable empty RAM zone(a). 
//!			Following this, place 1 movable page in an empty zone(b) ensuring that there is enough free space in 
//!			the more preferable RAM zones for the movable pages in both zones (a) and (b). 
//!			Following this call a general defrag. 
//!		4.	Allocate 1 movable and 1 discardable page into the least preferable empty RAM zone
//!			ensuring that there is not enough free space in the more preferable RAM zones for the
//!			movable pages in the RAM zone under test. Following this call a general defrag. 
//! 	5.	Fragment the memory. 
//! 		Following this allocate discardable pages by loading pages that are demand paged. 
//! 		Read each of the constants from beginning to end. 
//! 		Following this perform a RAM defrag. 
//! 	6.	Call a device driver that will continuously allocate fixed pages to the
//! 		memory until it reports out of memory. Following this perform a RAM defrag. 
//!		7.	Defrag memory filled with discardable pages when the min cache size is reached
//!		8.	Fragment the memory. Following this, continuously call a general defrag, each time 
//!			reducing the size of the chunks that are allocated  
//! 
//! @SYMTestExpectedResults
//! 	1.	The RAM zone under test is not emptied and all the discardable pages contained in it
//!			have not been discarded.
//! 	2.	The RAM zone under test is not emptied and all the discardable pages contained in it
//!			have not been discarded.
//! 	3.	RAM zones (a) and (b) have been emptied and all the discardable pages contained in them
//!			have not been discarded.
//!		4.	The RAM zone under test is emptied
//! 	5.	Pages are discarded
//! 	6.	No zones are emptied
//!		7.	The least preferable zone is skipped but the other zones are defragmented as required.
//! 	8.	Zones are emptied if the general defrag can succeed
//---------------------------------------------------------------------------------------------------------------------
TInt TestGenDefrag()
	{
	TInt r = KErrNone;
	
	test.Start(_L("Test1: Test General Defrag doesn't discard pages when fixed page in zone to be emptied"));	
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	TUint numFreeInUse = 0;
	TUint testNo = 1;

	// We just need 1 movable, 1 discardable and 1 fixed in the zone to ensure it cannot be emptied
	// as the fixed should block the defrag
	TInt movAllocPages = 1;
	TInt fixAllocPages = 1;
	TInt disAllocPages = 1;
	TUint zoneIndex;

	if (GenDefragTest123Setup(movAllocPages, fixAllocPages, disAllocPages, zoneIndex, numFreeInUse, testNo) != KErrNone)
		{
		test.Printf(_L("Setup failed - Skipping..\n"));
		goto skipTest1;
		}

	// Call a general defrag and check that no discardable pages were freed.
	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	GetAllPageInfo();
	if (r != KErrNone || 
		gZoneUtilArray[zoneIndex].iAllocMovable < gOriginalPageCountArray[zoneIndex].iAllocMovable ||
		gOriginalPageCountArray[zoneIndex].iAllocDiscardable > gZoneUtilArray[zoneIndex].iAllocDiscardable)
		{
		test.Printf(_L("Fail: r = %d zoneIndex %d origMov 0x%x curMov 0x%x origDis 0x%x curDis 0x%x\n"), r, zoneIndex, 
						gOriginalPageCountArray[zoneIndex].iAllocMovable, gZoneUtilArray[zoneIndex].iAllocMovable, 
						gOriginalPageCountArray[zoneIndex].iAllocDiscardable, gZoneUtilArray[zoneIndex].iAllocDiscardable);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}

skipTest1:
	// TestEnd() will perform any required clean up.
	TestEnd();

	test.Next(_L("Test2: Test General Defrag doesn't discard pages when zone can't be emptied"));	
	TestStart();	
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	testNo++;
	// No fixed, but there should be no space for the movable to go to
	// GenDefragTest123Setup(), will ensure that this is the case
	// by allocating 1 more movable page than there is free in the in-use zones
	movAllocPages = 1;
	fixAllocPages = 0;
	disAllocPages = 1;

	if (GenDefragTest123Setup(movAllocPages, fixAllocPages, disAllocPages, zoneIndex, numFreeInUse, testNo) != KErrNone)
		{
		test.Printf(_L("Setup failed - Skipping..\n"));
		goto skipTest2;
		}

	// Call a general defrag and check that no discardable pages were freed.
	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	
	GetAllPageInfo();
	if (r != KErrNone || 
		gZoneUtilArray[zoneIndex].iAllocMovable < gOriginalPageCountArray[zoneIndex].iAllocMovable ||
		gOriginalPageCountArray[zoneIndex].iAllocDiscardable > gZoneUtilArray[zoneIndex].iAllocDiscardable)
		{
		test.Printf(_L("Fail: r = %d zoneIndex %d origMov 0x%x curMov 0x%x origDis 0x%x curDis 0x%x\n"), r, zoneIndex, 
						gOriginalPageCountArray[zoneIndex].iAllocMovable, gZoneUtilArray[zoneIndex].iAllocMovable, 
						gOriginalPageCountArray[zoneIndex].iAllocDiscardable, gZoneUtilArray[zoneIndex].iAllocDiscardable);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}

skipTest2:
	// TestEnd() will perform any required clean up.
	TestEnd();

	test.Next(_L("Test3: Test General Defrag tidies when fixed page in zone to be emptied but other can be emptied"));	
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	gChunkArray2 = new RChunk;
	gChunkArraySize2 = 1;
	TUint totalToMove;
	testNo++;

	// First use GenDefragTest123Setup() to ensure we place 1 fixed, 1 movable and 1 discardable
	// in the least preferable zone
	movAllocPages = 1;
	fixAllocPages = 1;
	disAllocPages = 1;
	TBool zoneFound = EFalse;	
	TUint emptyZoneIndex = 0;

	if (GenDefragTest123Setup(movAllocPages, fixAllocPages, disAllocPages, zoneIndex, numFreeInUse, testNo) != KErrNone)
		{
		test.Printf(_L("Setup failed - Skipping..\n"));
		goto skipTest3;
		}

	// Find an empty zone and place a movable page in it
	GetAllPageInfo();
	for (TUint i = 0; i < gZoneCount; i++)
		{
		if (gZoneUtilArray[i].iFreePages == gZoneUtilArray[i].iPhysPages)
			{
			zoneFound = ETrue;
			emptyZoneIndex = i;
			break;
			}
		}
	if (!zoneFound)
		{
		test.Printf(_L("Can't find empty zone - Skipping..\n"));
		goto skipTest3;
		}

	// Allocate 1 movable page into the empty zone
	r = ZoneAllocMovable(gChunkArray2, gChunkArraySize2, emptyZoneIndex, 1);
	GetAllPageInfo();
	if (r != KErrNone ||
		gZoneUtilArray[emptyZoneIndex].iAllocMovable == 0 ||
		gZoneUtilArray[emptyZoneIndex].iAllocFixed != 0)
		{
		test.Printf(_L("Movable pages not allocated or fixed pages allocated. - Skipping...\n"));
		test.Printf(_L("zone mov 0x%x free 0x%x r =% d\n"), 
					gZoneUtilArray[emptyZoneIndex].iAllocMovable,	
					gZoneUtilArray[emptyZoneIndex].iFreePages, r);
		goto skipTest3;
		}

	// Check that the amount we are allocating can actually be moved into the in-use zones
	totalToMove = gZoneUtilArray[zoneIndex].iAllocMovable + gZoneUtilArray[zoneIndex].iAllocDiscardable + 
					gZoneUtilArray[emptyZoneIndex].iAllocMovable + gZoneUtilArray[emptyZoneIndex].iAllocDiscardable;
	
	// Check the total number of free pages in the in-use zones
	numFreeInUse = 0;
	for (TUint index = 0; index < gZoneCount; index++)
		{
		if (gZoneUtilArray[index].iFreePages != gZoneUtilArray[index].iPhysPages &&
			index != zoneIndex && index != emptyZoneIndex)
			{
			numFreeInUse += gZoneUtilArray[index].iFreePages;
			}
		}
	if(numFreeInUse < totalToMove)
		{
		test.Printf(_L("No space to move pages numFreeInUse = 0x%x totalToMove = 0x%x - Skipping..\n"), 
						numFreeInUse, totalToMove);
		goto skipTest3;
		}

	// Call a general defrag and check that zone is emptied.
	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	
	GetAllPageInfo();
	if (r != KErrNone || 
		gZoneUtilArray[zoneIndex].iAllocMovable ||
		gZoneUtilArray[zoneIndex].iAllocDiscardable ||
		gZoneUtilArray[emptyZoneIndex].iAllocMovable)
		{
		test.Printf(_L("Fail: r = %d zoneIndex %d origMov 0x%x curMov 0x%x origDis 0x%x curDis 0x%x\n"), r, zoneIndex, 
						gOriginalPageCountArray[zoneIndex].iAllocMovable, gZoneUtilArray[zoneIndex].iAllocMovable, 
						gOriginalPageCountArray[zoneIndex].iAllocDiscardable, gZoneUtilArray[zoneIndex].iAllocDiscardable);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}

skipTest3:
	// TestEnd() will perform any required clean up.
	TestEnd();

	test.Next(_L("Test4: Test General Defrag moves pages into the next most pref if they don't fit into the most pref"));	
	TestStart();
	gChunkArray1 = new RChunk;	
	gChunkArraySize1 = 1;
	testNo++;
	
	// No fixed, but there should be no space for the movable to go to
	// GenDefragTest123Setup(), will ensure that this is the case
	// by allocating 1 more movable page than there is free in the in-use zones
	movAllocPages = 1;
	fixAllocPages = 0;
	disAllocPages = 1;

	if (GenDefragTest123Setup(movAllocPages, fixAllocPages, disAllocPages, zoneIndex, numFreeInUse, testNo) != KErrNone)
		{
		test.Printf(_L("Setup failed - Skipping..\n"));
		goto skipTest4;
		}

	// Call a general defrag and check that the test zone is emptied.
	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	
	GetAllPageInfo();
	if (r != KErrNone || 
		gZoneUtilArray[zoneIndex].iFreePages != gZoneUtilArray[zoneIndex].iPhysPages)
		{
		test.Printf(_L("Fail: r = %d zoneIndex %d origMov 0x%x curMov 0x%x origDis 0x%x curDis 0x%x\n"), r, zoneIndex, 
						gOriginalPageCountArray[zoneIndex].iAllocMovable, gZoneUtilArray[zoneIndex].iAllocMovable, 
						gOriginalPageCountArray[zoneIndex].iAllocDiscardable, gZoneUtilArray[zoneIndex].iAllocDiscardable);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}

skipTest4:
	// TestEnd() will perform any required clean up.
	TestEnd();

	test.Next(_L("Test5: Defrag memory filled with discardable pages\n"));
	TestStart();
	if (gPagedRom)
		{
		AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
		FreeMovable(gChunkArray1, gChunkArraySize1);
		
		GetAllPageInfo();	
		TInt discardablePages;
		UpdateRamInfo();
		
		r = AllocDiscardable(discardablePages);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		TESTDEBUG(test.Printf(_L("Number of discardable pages = 0x%x\n"), discardablePages >> gPageShift));
		
		GetOriginalPageCount();
		TBool genSucceed = CanGenSucceed();
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		TESTDEBUG(test.Printf(_L("After calling defrag r = %d\n"), r));
		
		if (r != KErrNone)
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"),r,KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else if (genSucceed && CheckZonesSwitchedOff() == EFalse)
			{
			test.Printf(_L("No Zones Switched off and should have been\n"));
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}

		RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
		ResetDPCache();	
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();
		
	
	test.Next(_L("Test6: Defrag memory filled with fixed pages"));	
	TestStart();
	
	TESTDEBUG(test.Printf(_L("Filling memory with fixed pages, r = %d\n")));
	r = Ldd.AllocateFixed(FILL_ALL_FIXED);

	GetOriginalPageCount();
	TBool genSucceed = CanGenSucceed();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	TESTDEBUG(test.Printf(_L("After calling defrag r = %d\n"), r));

	// The zones should be full of fixed pages so the general should do nothing.
	if (r != KErrNone || 
		genSucceed || 
		CheckZonesSwitchedOff())
		{
		test.Printf(_L("Fail: r = %d, expected = %d, or zone have been emptied\n"), r, KErrNone);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	Ldd.FreeAllFixedPages();
	TestEnd();

	PrintPageInfo();

	test.Next(_L("Test7: Defrag memory filled with discardable pages when the min cache size is reached\n"));
	TestStart();
	if (gPagedRom)
		{
		AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
		FreeMovable(gChunkArray1, gChunkArraySize1);
		
		GetAllPageInfo();	
		TInt discardablePages;
		UpdateRamInfo();
		
		r = AllocDiscardable(discardablePages);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		TESTDEBUG(test.Printf(_L("Number of discardable pages = 0x%x\n"), discardablePages >> gPageShift));

		TUint minCacheSize = 0;
		TUint maxCacheSize = 0;
		TUint currentCacheSize = 0;

		DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);

		TUint setMin = maxCacheSize;
		TUint64 setMax = maxCacheSize;
		TInt r = DPTest::SetCacheSize(setMin, setMax);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}			
		TESTDEBUG(test.Printf(_L("After SetCacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x\n"), 
															setMin >> gPageShift, setMax >> gPageShift));
		
		GetOriginalPageCount();
		genSucceed = CanGenSucceed();
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		TESTDEBUG(test.Printf(_L("After calling defrag r = %d\n"), r));

		if (r != KErrNone)
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"),r,KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}
		else if (genSucceed && CheckZonesSwitchedOff() == EFalse)
			{
			test.Printf(_L("No Zones Switched off and should have been\n"));
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}

		RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
		ResetDPCache();
		
		
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();

	test.Next(_L("Test8: Defrag fragmented memory for various smaller chunk sizes"));	
	TInt chunkSize = 0x80000;
	while (chunkSize >= 0x4000)
		{
		test.Printf(_L("chunkSize = %dKB\n"), chunkSize/1024);
		TestStart();
		r = AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable, chunkSize, EFalse);
		r = FreeMovable(gChunkArray1, gChunkArraySize1);
		
		GetOriginalPageCount();
		TBool genSucceed = CanGenSucceed();
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		TESTDEBUG(test.Printf(_L("After calling defrag r = %d\n"), r));
		
		if (r != KErrNone)
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"),r,KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else if (genSucceed && CheckZonesSwitchedOff() == EFalse)
			{
			test.Printf(_L("No Zones Switched off and should have been\n"));
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}

		RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
		TestEnd();
		chunkSize = chunkSize>>1; 
		}

	test.Next(_L("Test9: Defrag fragmented memory "));	
	TestStart();
	
	r = AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable);
	r = FreeMovable(gChunkArray1, gChunkArraySize1);
	
	GetOriginalPageCount();
	genSucceed = CanGenSucceed();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	TESTDEBUG(test.Printf(_L("After calling defrag r = %d\n"), r));

	if (r != KErrNone)
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"),r,KErrNone);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else if (genSucceed && CheckZonesSwitchedOff() == EFalse)
		{
		test.Printf(_L("No Zones Switched off and should have been\n"));
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();

	test.End();
	return KErrNone;
	}


//
// TestGetRamZonePageCount
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0529
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the implementation of the function GetRamZonePageCount()
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Following this, call GetRamZonePageCount() on every zone and verify
//!			the values with the test HAL functions. 
//! 	2.	Fragment the memory. Following this, call function with an valid aID
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrNone
//! 	2.	KErrArgument
//---------------------------------------------------------------------------------------------------------------------
TInt TestGetRamZonePageCount()
	{
	test.Start(_L("Test1: Call GetRamZonePageCount() on every zone one after the other"));	
	TestStart();	
		
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	for (TUint index = 0; index < gZoneCount; index++)
		{
		GetAllPageInfo();
		TUint zoneID = gZoneConfigArray[index].iZoneId;

		STestUserSidePageCount pageData;
		TInt r = Ldd.PageCount(zoneID, &pageData);
		TESTDEBUG(test.Printf(_L("Page count function r = %d\n"), r));
		if (r != KErrNone)
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		
		if (pageData.iFreePages != gZoneUtilArray[index].iFreePages ||
			pageData.iFixedPages != gZoneUtilArray[index].iAllocFixed ||
			pageData.iMovablePages != gZoneUtilArray[index].iAllocMovable ||
			pageData.iDiscardablePages != gZoneUtilArray[index].iAllocDiscardable)
			{
			test.Printf(_L("RAM zone page count does not match test HAL page count, Zone %d\n"), zoneID);
			test.Printf(_L("PgCnt: free = 0x%x, fixed = 0x%x, movable= 0x%x, discard = 0x%x\n"),
									pageData.iFreePages, pageData.iFixedPages, pageData.iMovablePages, pageData.iDiscardablePages);
			test.Printf(_L("HalFunc: free = 0x%x, fixed = 0x%x, movable= 0x%x, discard = 0x%x\n"),
									gZoneUtilArray[index].iFreePages, gZoneUtilArray[index].iAllocFixed, 
									gZoneUtilArray[index].iAllocMovable, gZoneUtilArray[index].iAllocDiscardable);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Zone %d Passed...\n"), zoneID);	
			}
		TESTDEBUG(test.Printf(_L("iFreePages = 0x%x, iFixedPages = 0x%x, iMovablePages = 0x%x, iDiscardablePages = 0x%x\n"),
						pageData.iFreePages, pageData.iFixedPages, pageData.iMovablePages, pageData.iDiscardablePages));
		}	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();
	
	test.Next(_L("Test2: Call GetRamZonePageCount() with an invalid aID "));	
	TestStart();	
		
	TUint zoneID = KInvalidZoneID;
	STestUserSidePageCount pageData;
	TInt r = Ldd.PageCount(zoneID, &pageData);
	TESTDEBUG(test.Printf(_L("Page count function r = %d\n"), r));
	if (r != KErrArgument)
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	TestEnd();
	
	test.End();
	return 0;
	}

const TUint KIndex2Alloc = 10;
const TUint KTestMaxPages = 6;

//
// DefragMaxPagesSetup
//
// Arranges the memory layout in preparation for TestDefragRamMaxPages()
//
// WARNING THIS WILL BE UNRELIABLE IF aMaxPages > KIndex2Alloc
// 
TInt DefragMaxPagesSetup(TInt aPageType, TUint aMaxPages, TUint& aIndex, TUint& aIndex2)
	{
	TInt r = KErrNoMemory;
	
	// Get the preference order and determine the 2 most preferable zones
	// that are currently not in use.
	GetPrefOrder();
	TUint freeInUsePages = 0;
	TBool zonesFound = EFalse;
	for (TUint i = 1; i < gZoneCount; i++)
		{
		aIndex = gPrefArray[i];
		aIndex2 = gPrefArray[i-1];
		TUint indexFree = gZoneUtilArray[aIndex].iFreePages;
		TUint index2Free = gZoneUtilArray[aIndex2].iFreePages;
		if (indexFree == gZoneUtilArray[aIndex].iPhysPages &&
			index2Free == gZoneUtilArray[aIndex2].iPhysPages &&
			indexFree >= aMaxPages && index2Free >= KIndex2Alloc &&
			freeInUsePages >= KIndex2Alloc + aMaxPages)
			{
			zonesFound = ETrue;
			break;
			}
		freeInUsePages += index2Free;
		}	
	
	// Could suitable RAM zones be found.
	if (!zonesFound)
		{
		test.Printf(_L("Insufficient memory - Skipping test...\n"));
		return KErrNoMemory;
		}

	if (aPageType == BEST_MOVABLE)
		{		
		// Allocate KIndex2Alloc movable pages to aIndex2
		r = ZoneAllocMovable(gChunkArray1, gChunkArraySize1, aIndex2, KIndex2Alloc);
		if (r != KErrNone)
			{
			test.Printf(_L("Insufficient memory - Skipping test...\n"));
			r = KErrNoMemory;
			goto error;
			}

			

		// Now allow movable pages to be allocated into the least preferable RAM zone under test only.
		GetAllPageInfo();
		if (aMaxPages >= gZoneUtilArray[aIndex].iFreePages)
			{
			test.Printf(_L("Insufficient memory available - Skipping test...\n"));
			r = KErrNoMemory;
			goto error;
			}
		// Allocate aMaxPages movable pages to aIndex
		r = ZoneAllocMovable(gChunkArray2, gChunkArraySize2, aIndex, aMaxPages);
		if (r != KErrNone)
			{
			test.Printf(_L("Insufficient memory - Skipping test...\n"));
			r = KErrNoMemory;
			goto error;
			}
		
		// Determine how many free pages there are in the RAM zones more preferable
		// than the RAM zones under test.
		GetPrefOrder();
		freeInUsePages = 0;
		for (TUint i = 0; i < gZoneCount; i++)
			{
			TUint tmpIndex = gPrefArray[i];
			if (tmpIndex == aIndex2)
				break;
			freeInUsePages += gZoneUtilArray[tmpIndex].iFreePages;
			}
		// Verify that the RAM layout is still suitable for the test.
		if (gZoneUtilArray[aIndex].iAllocMovable != aMaxPages ||
			gZoneUtilArray[aIndex2].iAllocMovable != KIndex2Alloc ||
			gZoneUtilArray[aIndex].iAllocDiscardable || gZoneUtilArray[aIndex].iAllocFixed || 
			gZoneUtilArray[aIndex2].iAllocDiscardable || gZoneUtilArray[aIndex2].iAllocFixed ||
			freeInUsePages < KIndex2Alloc + aMaxPages)
			{
			test.Printf(_L("Insufficient memory - Skipping test...\n"));
			r = KErrNoMemory;
			goto error;
			}

		// now reset all the flags
		ResetRamZoneFlags();

		// Perform a general defrag
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, KTestMaxPages);
		goto exit;
		}
		
	if (aPageType == BEST_DISCARDABLE)
		{

		TUint minCacheSize = 0;
		TUint maxCacheSize = 0;
		TUint currentCacheSize = 0;
		DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
		test.Printf(_L("Original CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
									minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift);
					

	
		// Allocate a number of discardable pages to the 2nd least preferable zone under test
		TInt disPages;
		r = ZoneAllocDiscard(aIndex2, KIndex2Alloc, disPages);
		if (r != KErrNone)
			{
			test.Printf(_L("ZoneAllocDiscard() r = %d KIndex2Alloc = 0x%x disPages = 0x%x aIndex2 = %d - Skipping test...\n"),
							r, KIndex2Alloc, disPages, aIndex2);
			GetAllPageInfo();
			PrintPageInfo();
			r = KErrNoMemory;
			goto error;
			}
		TUint discFillBytes = KIndex2Alloc << gPageShift;
		r = DPTest::SetCacheSize(currentCacheSize + discFillBytes, currentCacheSize + discFillBytes);
		if (r != KErrNone)
			{
			test.Printf(_L("SetCacheSize r = 0x%x currentCacheSize + discFillBytes = 0x%x - Skipping test...\n"),
							r, currentCacheSize + discFillBytes);
			GetAllPageInfo();
			PrintPageInfo();
			r = KErrNoMemory;
			goto error;
			}

	
		// Allocate a discardable pages equal to aMaxPages to the least preferable zone under test
		GetAllPageInfo();
		if(aMaxPages >= gTotalPageCount.iFreePages)
			{
			test.Printf(_L("Insufficient memory available - Skipping test...\n"));
			r = KErrNoMemory;
			goto error;
			}
		TUint allocPages = aMaxPages + KIndex2Alloc;
		r = ZoneAllocDiscard(aIndex, aMaxPages, disPages);
		if (r != KErrNone)
			{
			test.Printf(_L("ZoneAllocDiscard() r = %d aMaxPages = 0x%x disPages = 0x%x aIndex = %d - Skipping test...\n"),
							r, aMaxPages, disPages, aIndex);
			GetAllPageInfo();
			PrintPageInfo();
			r = KErrNoMemory;
			goto error;
			}
		discFillBytes = allocPages << gPageShift;
		r = DPTest::SetCacheSize(currentCacheSize + discFillBytes, currentCacheSize + discFillBytes);
		if (r != KErrNone)
			{
			test.Printf(_L("SetCacheSize r = %d currentCacheSize + discFillBytes = 0x%x - Skipping test...\n"),
							r, currentCacheSize + discFillBytes);
			GetAllPageInfo();
			PrintPageInfo();
			r = KErrNoMemory;
			goto error;
			}


		// Determine how many free pages there are in the RAM zones more preferable
		// than the RAM zones under test.
		GetPrefOrder();
		freeInUsePages = 0;
		for (TUint i = 0; i < gZoneCount; i++)
			{
			TUint tmpIndex = gPrefArray[i];
			if (tmpIndex == aIndex2)
				break;
			freeInUsePages += gZoneUtilArray[tmpIndex].iFreePages;
			}
		// Verify that the RAM layout is still suitable for the test.
		if (r != KErrNone || gZoneUtilArray[aIndex].iAllocDiscardable != aMaxPages ||
			gZoneUtilArray[aIndex2].iAllocDiscardable != KIndex2Alloc ||
			gZoneUtilArray[aIndex].iAllocMovable || gZoneUtilArray[aIndex].iAllocFixed || 
			gZoneUtilArray[aIndex2].iAllocMovable || gZoneUtilArray[aIndex2].iAllocFixed ||
			freeInUsePages < KIndex2Alloc + aMaxPages)
			{
			test.Printf(_L("Insufficient memory - Skipping test...\n"));
			r = KErrNoMemory;
			goto error;
			}

		// now reset all the flags
		ResetRamZoneFlags();

		// Perform a general defrag with maxPages = KTestMaxPages
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, KTestMaxPages);
		goto exit;

		}
error:
	// Reset all the flags
	ResetRamZoneFlags();
exit:
	return r;
	}

//
// TestDefragRamMaxPages
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0530
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the implementation of the function TRamDefragRequest::DefragRam()
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Call function with invalid aMaxPages 
//! 	2.	Call DefragRam when aMaxPages < number of movable pages in least preferable zone 
//! 	3.	Call DefragRam when aMaxPages > number of movable pages in least preferable zone
//! 	4.	Call DefragRam when aMaxPages = number of movable pages in least preferable zone
//! 	5.	Call DefragRam when aMaxPages < number of discardable pages in least preferable zone 
//! 	6.	Call DefragRam when aMaxPages > number of discardable pages in least preferable zone
//! 	7.	Call DefragRam when aMaxPages = number of discardable pages in least preferable zone
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrArgument
//! 	2.	Least preferable zone and 2nd least preferable zone have not been emptied
//! 	3.	Least preferable zone has been emptied and 2nd least preferable zone has not been emptied
//! 	4.	Least preferable zone has been emptied and 2nd least preferable zone not not been emptied
//! 	5.	Least preferable zone and 2nd least preferable zone have not been emptied
//! 	6.	Least preferable zone has been emptied and 2nd least preferable zone has not been emptied
//! 	7.	Least preferable zone has been emptied and 2nd least preferable zone not not been emptied
//---------------------------------------------------------------------------------------------------------------------
TInt TestDefragRamMaxPages()
	{
	test.Start(_L("Test1: Call DefragRam with invalid aMaxPages "));	
	TestStart();	
	
	gDefragMaxPages = -1;
	
	TInt r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);
	TESTDEBUG(test.Printf(_L("After calling defrag r = %d\n"), r));
	if (r != KErrArgument)
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}	
	TestEnd();
	
	test.Next(_L("Test2: Call DefragRam aMaxPages < number of movable pages in least preferable zone "));	
	TestStart();	
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	gChunkArray2 = new RChunk;
	gChunkArraySize2 = 1;
	TUint index = 0;
	TUint index2 = 0;
	r = DefragMaxPagesSetup(BEST_MOVABLE, KTestMaxPages + 1, index, index2);	
	if (gZoneUtilArray[index].iAllocFixed > 0 || gZoneUtilArray[index2].iAllocFixed > 0)
		{// Cannot perform test with fixed pages in least preferable zones
		test.Printf(_L("Fixed pages in least preferable zone - Skipping...\n"));
		}
	else
		{
		if (r == KErrNone)
			{
			GetAllPageInfo();
			if (gZoneUtilArray[index].iAllocMovable < KTestMaxPages + 1 ||
				gZoneUtilArray[index2].iAllocMovable < KIndex2Alloc)
				{
				test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
										index, KTestMaxPages + 1, index2, KIndex2Alloc);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
				CLEANUP(Ldd.FreeFromAllZones());
				TEST_FAIL;
				}

			// Double check that if DefragRam is called with maxPages = 0, these zones are emptied
			// if there is free space elsewhere
			r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);
			GetAllPageInfo();
			if (gTotalPageCount.iFreePages > KIndex2Alloc + KTestMaxPages + 1 &&
				(gZoneUtilArray[index].iAllocMovable != 0 || gZoneUtilArray[index2].iAllocMovable != 0))
				{
				test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
										index, 0, index2, 0);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
				CLEANUP(Ldd.FreeFromAllZones());
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));
				}
			}
		else 
			{
			test.Printf(_L("DefragMaxPagesSetup failed r = %d\n"), r);
			}
		RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
		RemoveChunkAlloc(gChunkArray2, gChunkArraySize2);
		}
	Ldd.FreeFromAllZones();
	TestEnd();


	test.Next(_L("Test3: Call DefragRam aMaxPages > number of movable pages in least preferable zone "));	
	TestStart();	
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	gChunkArray2 = new RChunk;
	gChunkArraySize2 = 1;
	r = DefragMaxPagesSetup(BEST_MOVABLE, KTestMaxPages - 1, index, index2);
	if (gZoneUtilArray[index].iAllocFixed > 0 || gZoneUtilArray[index2].iAllocFixed > 0)
		{// Cannot perform test with fixed pages in least preferable zones
		test.Printf(_L("Fixed pages in least preferable zone - Skipping...\n"));
		}
	else
		{		
		if (r == KErrNone)
			{
			GetAllPageInfo();
			if (gZoneUtilArray[index].iAllocMovable != 0)
				{
				test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
										index, 0, index2, KIndex2Alloc);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
				CLEANUP(Ldd.FreeFromAllZones());
				TEST_FAIL;		
				}
			
			// Double check that if DefragRam is called with maxPages = 0, these zones are emptied
			// if there is free space elsewhere
			r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);
			
			GetAllPageInfo();
			if (gTotalPageCount.iFreePages > KIndex2Alloc + KTestMaxPages - 1 &&
				(gZoneUtilArray[index].iAllocMovable != 0 || gZoneUtilArray[index2].iAllocMovable != 0))
				{
				test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
										index, 0, index2, 0);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
				CLEANUP(Ldd.FreeFromAllZones());
				TEST_FAIL;		
				}
			else
				{
				test.Printf(_L("Passed...\n"));
				}
			}
		else 
			{
			test.Printf(_L("DefragMaxPagesSetup failed r = %d\n"), r);
			}	
		
		RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
		RemoveChunkAlloc(gChunkArray2, gChunkArraySize2);
		}
	Ldd.FreeFromAllZones();
	TestEnd();


	test.Next(_L("Test4: Call DefragRam aMaxPages = number of movable pages in least preferable zone "));	
	TestStart();	
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	gChunkArray2 = new RChunk;
	gChunkArraySize2 = 1;
	r = DefragMaxPagesSetup(BEST_MOVABLE, KTestMaxPages, index, index2);
	if (gZoneUtilArray[index].iAllocFixed > 0 || gZoneUtilArray[index2].iAllocFixed > 0)
		{// Cannot perform test with fixed pages in least preferable zones
		test.Printf(_L("Fixed pages in least preferable zone - Skipping...\n"));
		}
	else
		{		
		if (r == KErrNone)
			{
			GetAllPageInfo();
			if (gZoneUtilArray[index].iAllocMovable != 0)
				{
				test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
										index, 0, index2, KIndex2Alloc);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
				CLEANUP(Ldd.FreeFromAllZones());
				TEST_FAIL;		
				}

			// Double check that if DefragRam is called with maxPages = 0, these zones are emptied
			// if there is free space elsewhere
			r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);
			
			GetAllPageInfo();
			if (gTotalPageCount.iFreePages > KIndex2Alloc + KTestMaxPages &&
				(gZoneUtilArray[index].iAllocMovable != 0 || gZoneUtilArray[index2].iAllocMovable != 0))
				{
				test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
										index, 0, index2, 0);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(RemoveChunkAlloc(gChunkArray2, gChunkArraySize2));
				CLEANUP(Ldd.FreeFromAllZones());
				TEST_FAIL;		
				}
			else 
				{
				test.Printf(_L("Passed...\n"));
				}
			}
		else 
			{
			test.Printf(_L("DefragMaxPagesSetup failed r = %d\n"), r);
			}
		RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
		RemoveChunkAlloc(gChunkArray2, gChunkArraySize2);
		}
	Ldd.FreeFromAllZones();
	TestEnd();
	
	test.Next(_L("Test5: Call DefragRam aMaxPages < number of discardable pages in least preferable zone "));	
	TestStart();	
	if (gPagedRom)
		{
		r = DefragMaxPagesSetup(BEST_DISCARDABLE, KTestMaxPages + 1, index, index2);
		if (gZoneUtilArray[index].iAllocFixed > 0 || gZoneUtilArray[index2].iAllocFixed > 0)
			{// Cannot perform test with fixed pages in least preferable zones
			test.Printf(_L("Fixed pages in least preferable zone - Skipping...\n"));
			}
		else
			{
			if (r == KErrNone)
				{
				GetAllPageInfo();
				if (gZoneUtilArray[index].iAllocDiscardable < KTestMaxPages + 1 ||
					gZoneUtilArray[index2].iAllocDiscardable < KIndex2Alloc)
					{
					test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
											index, KTestMaxPages + 1, index2, KIndex2Alloc);
					CLEANUP(ResetDPCache());
					CLEANUP(Ldd.FreeFromAllZones());
					TEST_FAIL;		
					}
				
				// Double check that if DefragRam is called with maxPages = 0, these zones are emptied
				r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);
				
				GetAllPageInfo();
				if (gZoneUtilArray[index].iAllocDiscardable != 0 ||
					gZoneUtilArray[index2].iAllocDiscardable != 0)
					{
					test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
											index, 0, index2, 0);
					CLEANUP(ResetDPCache());
					CLEANUP(Ldd.FreeFromAllZones());
					TEST_FAIL;		
					}
				else
					{
					test.Printf(_L("Passed...\n"));
					}
				}
			else 
				{
				test.Printf(_L("DefragMaxPagesSetup failed r = %d\n"), r);
				}
			ResetDPCache();
			}
		Ldd.FreeFromAllZones();
		}
	else
		{
		test.Printf(_L("Not a Paged Rom - Skipping...\n"));
		}
	TestEnd();

	test.Next(_L("Test6: Call DefragRam aMaxPages > number of discardable pages in least preferable zone "));	
	TestStart();	
	
	if (gPagedRom)
		{
		r = DefragMaxPagesSetup(BEST_DISCARDABLE, KTestMaxPages - 1, index, index2);
		
		if (gZoneUtilArray[index].iAllocFixed > 0 || gZoneUtilArray[index2].iAllocFixed > 0)
			{// Cannot perform test with fixed pages in least preferable zones
			test.Printf(_L("Fixed pages in least preferable zone - Skipping...\n"));
			}
		else
			{	
			if (r == KErrNone)
				{
				GetAllPageInfo();
				if (gZoneUtilArray[index].iAllocDiscardable != 0||
					gZoneUtilArray[index2].iAllocDiscardable != KIndex2Alloc)
					{
					test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
											index, 0, index2, KIndex2Alloc);
					CLEANUP(ResetDPCache());
					CLEANUP(Ldd.FreeFromAllZones());
					TEST_FAIL;		
					}
				
				// Double check that if DefragRam is called with maxPages = 0, these zones are emptied
				r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);
				GetAllPageInfo();
				if (gZoneUtilArray[index].iAllocDiscardable != 0||
					gZoneUtilArray[index2].iAllocDiscardable != 0)
					{
					test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
											index, 0, index2, 0);
					CLEANUP(ResetDPCache());
					CLEANUP(Ldd.FreeFromAllZones());
					TEST_FAIL;		
					}
				else
					{
					test.Printf(_L("Passed...\n"));
					}
				}
			else 
				{
				test.Printf(_L("DefragMaxPagesSetup failed r = %d\n"), r);
				}	
			ResetDPCache();
			}
		Ldd.FreeFromAllZones();
		}
	else
		{
		test.Printf(_L("Not a Paged Rom - Skipping...\n"));
		}
	TestEnd();


	test.Next(_L("Test4: Call DefragRam aMaxPages = number of discardable pages in least preferable zone "));	
	TestStart();	
	
	if(gPagedRom)
		{
		r = DefragMaxPagesSetup(BEST_DISCARDABLE, KTestMaxPages, index, index2);
		
		if (gZoneUtilArray[index].iAllocFixed > 0 || gZoneUtilArray[index2].iAllocFixed > 0)
			{// Cannot perform test with fixed pages in least preferable zones
			test.Printf(_L("Fixed pages in least preferable zone - Skipping...\n"));
			}
		else
			{	
			if (r == KErrNone)
				{
				GetAllPageInfo();
				if (gZoneUtilArray[index].iAllocDiscardable != 0||
					gZoneUtilArray[index2].iAllocDiscardable != KIndex2Alloc)
					{
					test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
											index, 0, index2, KIndex2Alloc);
					CLEANUP(ResetDPCache());
					CLEANUP(Ldd.FreeFromAllZones());
					TEST_FAIL;		
					}

				// Double check that if DefragRam is called with maxPages = 0, these zones are emptied
				r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);
				GetAllPageInfo();
				if (gZoneUtilArray[index].iAllocDiscardable != 0||
					gZoneUtilArray[index2].iAllocDiscardable != 0)
					{
					test.Printf(_L("Fail: index = %d, expected = 0x%x, index2 = %d, expected = 0x%x\n"), 
											index, 0, index2, 0);
					CLEANUP(ResetDPCache());
					CLEANUP(Ldd.FreeFromAllZones());
					TEST_FAIL;		
					}
				else
					{
					test.Printf(_L("Passed...\n"));
					}
				}
			else 
				{
				test.Printf(_L("DefragMaxPagesSetup failed r = %d\n"), r);
				}
			ResetDPCache();
			}
		Ldd.FreeFromAllZones();
		}
	else
		{
		test.Printf(_L("Not a Paged Rom - Skipping...\n"));
		}
	TestEnd();

	test.End();
	return KErrNone;
	}


//
// TestEmptyRamZone
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0531
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the implementation of the function TRamDefragRequest::EmptyRamZone()
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Call function with a valid aID, 
//! 		i.e. a zone which lies within the range of zones. 
//! 	2.	Fragment the memory. Call function with an invalid aID
//! 		i.e. a zone which does not exist within the range of zones. 
//! 	3.	Fragment the memory. Following this, call a zone specific defrag on a particular zone. 
//! 		Call the zone specific defrag again on the same zone whilst the other defrag is still 
//!			running on that zone. 
//! 	4.	Fragment the memory. Following this, call the TRamDefragRequest::DefragRam() to perform general defrag. 
//! 		Following this call zone specific defrag whilst the general defrag is still running. 
//! 	5.	Fragment the memory. Following this, call a zone specific defrag on a particular zone. 
//! 		Whilst the zone defrag is running, call a general defrag
//! 	6:  Fragment the memory. Following this, call the function on specifc zone and 
//! 		at the same time allocate pages to the zone
//! 	7.	Fragment the memory. Call EmptyRamZone() on every zone one after the other. 
//! 	8.	Fragment the memory. Allocate a couple of fixed pages to a zone that contains movable pages. 
//! 		Following this, Call EmptyRamZone() on that zone. 
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrNone
//! 	2.	KErrArgument
//! 	3.	KErrNone and the zone has been emptied
//! 	4.	Zone has not been emptied, as the general defrag has already placed pages in the zone
//! 	5.	Zone has been emptied
//! 	6.	KErrNone and pages have been allocated in the zone
//! 	7.	One of the following scenarios should occur:
//!			a.	If the cache has not reached its minimum size, then all the movable pages should 
//!				be moved from the zone if there are enough free pages in the other zones. 
//!				However, when demand paging is off, all movable pages should be moved from the zone, again if there 
//!				are enough free pages in the other zones. 
//!			b.	If the current size of the cache minus the number of discardable pages in the zone being emptied is 
//!				not less than the minimum cache size, then all the discardable pages should be discarded of. 
//!				However, when demand paging is off, all discardable pages should be removed from 
//!				the zone. 
//! 	8.	KErrNoMemory, however all movable and discardable pages have been moved from the zone 
//---------------------------------------------------------------------------------------------------------------------
TInt TestEmptyRamZone()
	{
	TInt r = KErrNone;
	TInt r2 = KErrNone;

	test.Start(_L("Test1: Call EmptyRamZone with valid aID "));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	TUint defragZoneID = gZoneConfigArray[index].iZoneId;
	test.Printf(_L("Zone ID = %d\n"), defragZoneID);
	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, defragZoneID);
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && (r != KErrNone || !CheckZoneIsOff(index)))
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);		
	TestEnd();
	
	
	test.Next(_L("Test2: Call EmptyRamZone with invalid aID "));	
	TestStart();	
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	defragZoneID = KInvalidZoneID; 

	r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, defragZoneID);
	
	if (r != KErrArgument)
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	
	TestEnd();
	
	
	test.Next(_L("Test3: Call EmptyRamZone twice at the same time "));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = gTestThread.Create(gTestThreadName,MultiEmptyZoneThreadFunc,KDefaultStackSize,0x1000,0x1000,(TAny *)defragZoneID);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}		
				
		gTestThread.Logon(status);
		gTestThread.Resume();
		
		User::After(10);

		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, defragZoneID);
		
		User::WaitForRequest(status);
		r2 = status.Int();

		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (r != r2 || 
			(freeInOthers && (r != KErrNone || r2 != KErrNone || !CheckZoneIsOff(index))))
			{
			test.Printf(_L("Fail: r = %d, r2 = %d, expected = %d, or zone %d has not been emptied\n"), 
									r, r2, KErrNone, defragZoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));	
			TEST_FAIL;
			}	
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		
		gTestThread.Close();	
		}
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);		
	TestEnd();

		
	test.Next(_L("Test4: Call general defrag and zone defrag at the same time "));	
	TestStart();	
		
	TInt waitTime = 10000;
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && gZoneUtilArray[index].iAllocFixed > 0)
		{
		index--;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && gZoneUtilArray[index].iAllocFixed > 0)
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = gTestThread.Create(gTestThreadName,MultiGenDefragThreadFunc,KDefaultStackSize,0x1000,0x1000,NULL);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}		
		gTestThread.Logon(status);
				
		gTestThread.Resume();
		User::After(waitTime);
		
		TESTDEBUG(test.Printf(_L("Zone defrag running on main thread\n")));

		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, defragZoneID);

		User::WaitForRequest(status);
		r2 = status.Int();

		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (r2 != KErrNone || 
			(freeInOthers && (r != KErrNone || !CheckZoneIsOff(index))))
			{
			test.Printf(_L("Fail: r = %d, r2 = %d, expected = %d, or zone %d is on unexpectedly\n"),
							r, r2, KErrNone, defragZoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));	
			TEST_FAIL;
			}	
		else
			{
			test.Printf(_L("Passed...\n"));	
			}	

		gTestThread.Close();	
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();
	
	
	test.Next(_L("Test5: Call zone defrag and general defrag at the same time "));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;

	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = gTestThread.Create(gTestThreadName,MultiEmptyZoneThreadFunc,KDefaultStackSize,0x1000,0x1000,(TAny*)defragZoneID);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}		
		gTestThread.Logon(status);
				
		gTestThread.Resume();
		User::After(0);
		
		TESTDEBUG(test.Printf(_L("Zone defrag running on main thread\n")));
		
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, gDefragMaxPages);

		User::WaitForRequest(status);
		r2 = status.Int();

		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (r != KErrNone ||
			(freeInOthers && (r2 != KErrNone || !CheckZoneIsOff(index))))
			{
			test.Printf(_L("Fail: r = %d, r2 = %d, expected = %d, or zone 0x%x is on unexpectedly\n"),
							r, r2, KErrNone, defragZoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}

		gTestThread.Close();	
		}
			
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test6: Call function TRamDefragRequest::EmptyRamZone on specifc zone and at the same time allocate pages to the zone"));	
	TestStart();	
	
	TInt pagesAlloc = 1; // Try and allocate just one page whilst trying to empty the zone. 

	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		TInt numPassed = 0;
		for (waitTime = 1000; waitTime > 0; waitTime-=10)
			{
			r = gTestThread.Create(gTestThreadName,MultiEmptyZoneThreadFunc,KDefaultStackSize,0x1000,0x1000,(TAny*)defragZoneID);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}
			gTestThread.Logon(status);
			
			gTestThread.Resume();
			User::After(waitTime);
			
			r = Ldd.ZoneAllocDiscontiguous(defragZoneID, pagesAlloc);

			User::WaitForRequest(status);
			r2 = status.Int();
			
			if (r2 != KErrNone)
				{
				test.Printf(_L("Empty was unsuccsessful: r2 = %d, expected = %d\n"), r2, KErrNone);
				}
			else if (r != KErrNone)
				{
				test.Printf(_L("Fail: r = %d, expected = %d, r2 = %d, expected = %d, zone = 0x%x\n"), 
										r, KErrNone, r2, KErrNone, defragZoneID);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(Ldd.FreeAllFixedPages());
				TEST_FAIL;
				}
			else
				{
				numPassed ++;
				}
							
			Ldd.FreeAllFixedPages();

			gTestThread.Close();	
			}
		if (numPassed > 0)
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
			
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test7: Call function TRamDefragRequest::EmptyRamZone on every zone one after the other"));	
	TestStart();
		
	Ldd.ResetDriver();
			
	for (index = 0; index < gZoneCount; index ++)
		{
		// Variables for DP ROM cache sizes
		TUint minCacheSize = 0;
		TUint maxCacheSize = 0;
		TUint currentCacheSize = 0;

		if (gPagedRom)
			{
			DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
			TESTDEBUG(test.Printf(_L("Original CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
										minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
			// Calculate the maximum size available for the DP cache.
			TUint minZoneFree = KMaxTUint;
			TUint maxDisFillBytes = 0;
			GetAllPageInfo();
			for (TUint i = 0; i < gZoneCount; i++)
				{
				maxDisFillBytes += gZoneUtilArray[i].iFreePages;
				maxDisFillBytes += gZoneUtilArray[i].iAllocDiscardable;
				if (minZoneFree > gZoneUtilArray[i].iFreePages)
					minZoneFree = gZoneUtilArray[i].iFreePages;
				}
			test.Printf(_L("Free pages 0x%x maxDisFillBytes 0x%x\n"), gTotalPageCount.iFreePages, maxDisFillBytes);
			maxDisFillBytes <<= gPageShift;

			r = DPTest::SetCacheSize(maxDisFillBytes, maxDisFillBytes);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			
			DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
			TESTDEBUG(test.Printf(_L("CacheSize2: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
										minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
			if (currentCacheSize != maxDisFillBytes)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			
			GetAllPageInfo();
			test.Printf(_L("Free pages after alloc discardable1 = 0x%x\n"), gTotalPageCount.iFreePages);

			r = DPTest::SetCacheSize(minZoneFree << gPageShift, maxDisFillBytes);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			
			// Check all zones have no free pages.
			GetAllPageInfo();
			for (TUint i = 0; i < gZoneCount; i++)
				{
				if (gZoneUtilArray[i].iFreePages != 0)
					{
					test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
					CLEANUP(ResetDPCache());
					TEST_FAIL;
					}
				}
			}
		else
			{
			// Fragment the RAM with some movable pages
			AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable);
			FreeMovable(gChunkArray1, gChunkArraySize1);
			}
		
		TUint zoneID = gZoneConfigArray[index].iZoneId;
		
		GetAllPageInfo();
		TUint origUnknownInZone = gZoneUtilArray[index].iAllocUnknown;
		TUint origMovInZone = gZoneUtilArray[index].iAllocMovable;
		TUint origFreeInZone = gZoneUtilArray[index].iFreePages;

		test.Printf(_L("Zone ID 0x%x - fixedPages = 0x%x, unknownPages = 0x%x, discardablePages = 0x%x movable = 0x%x free = 0x%x\n"), 
						zoneID, gZoneUtilArray[index].iAllocFixed, origUnknownInZone, gZoneUtilArray[index].iAllocDiscardable, 
						origMovInZone, origFreeInZone);


		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, zoneID);

		GetAllPageInfo();		
		TUint unknownPages = gZoneUtilArray[index].iAllocUnknown;
		TUint discPages = gZoneUtilArray[index].iAllocDiscardable;
		TUint movablePages = gZoneUtilArray[index].iAllocMovable;
	
		TUint freeInOtherZones = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if(movablePages && freeInOtherZones)
			{
			test.Printf(_L("Fail: Zone ID %x all the movable pages haven't been moved\n"), zoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}

		if (origUnknownInZone != unknownPages)
			{
			test.Printf(_L("Fail: Zone ID %x unknown pages before and after are not equal\n"), zoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}

		if (!gPagedRom)
			{
			if (discPages && freeInOtherZones)
				{
				test.Printf(_L("Fail: Zone ID %x all the discardable pages haven't been moved\n"), zoneID);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			}
		else
			{
			DPTest::CacheSize(minCacheSize, maxCacheSize, currentCacheSize);
			TESTDEBUG(test.Printf(_L("CacheSize3: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
										minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
			// The the discardable pages should have been discarded or moved unless 
			// there is no room in the other zones and the cache size is already at its minimum
			if (discPages && (freeInOtherZones || currentCacheSize != minCacheSize))
				{
				test.Printf(_L("Fail: Zone 0x%x all the discardable pages haven't been moved\n"), zoneID);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			}
		test.Printf(_L("Passed...\n"));
		}
	// TestEnd() willl cleanup what's required
	TestEnd();

	test.Next(_L("Test8: Call function TRamDefragRequest::EmptyRamZone on a zone that contains fixed and movable pages"));
	TestStart();
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();	
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocMovable < 10 || gZoneUtilArray[index].iFreePages < 3))
		{	
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && (gZoneUtilArray[index].iAllocMovable < 10 || gZoneUtilArray[index].iFreePages < 3))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		test.Printf(_L("defragZoneID = 0x%x\n"), defragZoneID);		
		test.Printf(_L("movable = 0x%x discardable = 0x%x\n"), 
						gZoneUtilArray[index].iAllocMovable, gZoneUtilArray[index].iAllocDiscardable);	

		
		Ldd.ZoneAllocDiscontiguous(defragZoneID, 2); // Allocated 2 fixed pages to ensure that zone cannot be emptied
		
		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, defragZoneID);
		
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (r != KErrNoMemory || 
			(freeInOthers && 
			(gZoneUtilArray[index].iAllocMovable ||	
			gZoneUtilArray[index].iAllocDiscardable)))
			{
			test.Printf(_L("r = %d, expected = %d, or all movable/discardable pages have not been moved"), 
							r, KErrNoMemory);
			CLEANUP(Ldd.FreeAllFixedPages());
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed..."));
			}
		
		Ldd.FreeAllFixedPages();
		}
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);		
	TestEnd();

	test.End();
	return r;
	}


//
// TestClaimRamZone
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0532
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the implemntation of the function TRamDefragRequest::ClaimRamZone()
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Following this, call function with a valid aID
//! 	2.	Fragment the memory. Following this, call function with an invalid aID
//! 	3.	Fragment the memory. Following this, start a zone defrag and whilst this 
//! 		is running, call function on the same zone. 
//! 	4.	Fragment the memory. Following this, start a general defrag and whilst this 
//! 		is running, call function on the same zone.
//! 	5.	Fragment the memory. Following this, call ClaimRamZone() on a specific zone, whilst at the
//! 		same time calling ClaimRamZone() on another zone. 
//! 	6:	Fragment the memory. Call function TRamDefragRequest::ClaimRamZone on specifc zone and at the 
//! 		same time allocate pages to the zone
//!		7.	Fragment the memory. Following this allocate fixed pages to a zone and attempt to claim
//!			the zone. 
//!		8.	Fragment the memory. Following this call ClaimRamZone() on every zone, one after the other
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrNone
//! 	2.	KErrArgument
//! 	3.	KErrNone and the zone has been claimed
//! 	4.	KErrNone and the zone has been claimed
//! 	5.	KErrNone and both zones have been claimed
//! 	6.	KErrNoMemory
//!		7.	KErrNoMemory, all movable and discardable pages have been moved
//!		8.	One of the following scenarios should occur:
//!			a.	If the cache has not reached its minimum size and there are no unknown pages in 
//!				the zone, then all the movable pages should be moved from the zone if there are enough free pages
//!				in the other zones. 
//!				However, when demand paging is off, all movable pages should be moved from the zone, again if there 
//!				are enough free pages in the other zones and if there are no unknown pages in the zone. 
//!			b.	If the current size of the cache minus the number of discardable pages in the zone being emptied is not 
//!				less than the minimum cache size and there are no unknown pages in the zone, 
//!				then all the discardable pages should be discarded of. 
//!				However, when demand paging is off, all discardable pages should be removed from 
//!				the zone if there are no unknown pages in the zone. 
//---------------------------------------------------------------------------------------------------------------------
TInt TestClaimRamZone()
	{
	TInt r = 0;
	TInt r2 = 0;
	TUint32 addr = 0;
	TInt waitTime = 1000;
	
	test.Start(_L("Test1: Call function TRamDefragRequest::ClaimRamZone with a valid aID"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	TUint defragZoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, defragZoneID);
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && r != KErrNone && 
			gZoneUtilArray[index].iAllocFixed != gZoneUtilArray[index].iPhysPages)
			{
			test.Printf(_L("Fail: Zone 0x%x has not been claimed r %d expected %d\n"), defragZoneID, r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}

		addr = gZoneConfigArray[index].iPhysBase;
		if (r == KErrNone)
			{
			r = Ldd.FreeAllFixedPages();
			}
		}
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);		
	TestEnd();


	test.Next(_L("Test2: Call function TRamDefragRequest::ClaimRamZone with an invalid aID"));	
	TestStart();	
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	defragZoneID = KInvalidZoneID;
	
	r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, defragZoneID);	
	if (r != KErrArgument)
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}	
	
	TestEnd();


	test.Next(_L("Test3: Call function TRamDefragRequest::ClaimRamZone when a EmptyRamZone is already running"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;

	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		TInt numPassed = 0;
		for (waitTime = 1000; waitTime > 0; waitTime-=10)
			{
			r = gTestThread.Create(gTestThreadName,MultiEmptyZoneThreadFunc,KDefaultStackSize,0x1000,0x1000,(TAny*)defragZoneID);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}

			gTestThread.Logon(status);
			gTestThread.Resume();
			User::After(waitTime);
			
			r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, defragZoneID);

			User::WaitForRequest(status);
			r2 = status.Int();
			
			GetAllPageInfo();
			TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
			if (freeInOthers && r != KErrNone && r2 != KErrNone)
				{
				test.Printf(_L("Fail: r = %d, r2 = %d, expected = %d, or zone ID 0x%x has not been claimed\n"), 
										r, r2, KErrNone, defragZoneID);
				if (r == KErrNone)
					{
					CLEANUP(r = Ldd.FreeAllFixedPages());
					}	
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}
			else
				{
				numPassed ++;
				}
			
			if (r == KErrNone)
				{
				r = Ldd.FreeAllFixedPages();
				}		
			
			gTestThread.Close();	
			}
		if (numPassed > 0)
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test4: Call function TRamDefragRequest::ClaimRamZone when a general defrag is already running"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}

	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		defragZoneID = gZoneConfigArray[index].iZoneId;

		r = gTestThread.Create(gTestThreadName,MultiGenDefragThreadFunc,KDefaultStackSize,0x1000,0x1000,NULL);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		
		gTestThread.Logon(status);
		gTestThread.Resume();
		User::After(waitTime);

		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, defragZoneID);
		
		User::WaitForRequest(status);
		r2 = status.Int();
		
		if (r != KErrNone)
			{
			test.Printf(_L("ClaimZone: r = %d, expected = %d\n"), r, KErrNone);
			}	
		
		if (r2 != KErrNone)
			{
			test.Printf(_L("General: r2 = %d, expected = %d\n"), r, KErrNone);
			}

		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && gZoneUtilArray[index].iAllocFixed != gZoneUtilArray[index].iPhysPages)
			{
			test.Printf(_L("Fail: Zone ID 0x%x has not been claimed, r = %d, r2 = %d, expected = %d\n"), 
										defragZoneID, r, r2, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));	
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}

		if (r == KErrNone)
			{
			r = Ldd.FreeAllFixedPages();
			}
					
		gTestThread.Close();	
		}
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);		
	TestEnd();



	test.Next(_L("Test5: Call function TRamDefragRequest::ClaimRamZone on specifc zone at the same time as calling on another zone"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;
	test.Printf(_L("index = 0x%x "), index);

	TUint index2 = gZoneCount - 1;
	while (	index2 > 0 &&  
			(index == index2 || gZoneUtilArray[index2].iAllocFixed != 0 || gZoneUtilArray[index2].iAllocUnknown != 0))
		{
		-- index2;
		}
	TUint defragZoneID2 = gZoneConfigArray[index2].iZoneId;
	test.Printf(_L("index2 = %d\n"), index2);

	if ((index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) || 
		(index2 == 0 && (index == index2 || gZoneUtilArray[index2].iAllocFixed != 0 || gZoneUtilArray[index2].iAllocUnknown != 0)))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = gTestThread.Create(gTestThreadName,MultiClaimZoneThreadFunc,KDefaultStackSize,0x1000,0x1000,(TAny*)(defragZoneID2));
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}

		gTestThread.Logon(status);
		gTestThread.Resume();
		User::After(waitTime);
		
		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, defragZoneID);
		TESTDEBUG(test.Printf(_L("r = %d\n"), r));

		User::WaitForRequest(status);
		r2 = status.Int();
		
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		TUint freeInOthers2 = gTotalPageCount.iFreePages - gZoneUtilArray[index2].iFreePages;
		if ((freeInOthers && r != KErrNone && 
			gZoneUtilArray[index].iAllocFixed != gZoneUtilArray[index].iPhysPages) ||
			(freeInOthers2 && r != KErrNone && 
			gZoneUtilArray[index2].iAllocFixed != gZoneUtilArray[index2].iPhysPages))
			{
			test.Printf(_L("Fail: Zone ID 0x%x or Zone ID 0x%x has not been claimed, r = %d, r2 = %d, expected = %d\n"), 
									defragZoneID, defragZoneID2, r, r2, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		
		if (r == KErrNone)
			{
			r = Ldd.FreeAllFixedPages();
			}

		if (r2 == KErrNone)
			{// Have to free from specific address as RAM zone claimed by other channel.
			addr = gZoneConfigArray[index2].iPhysBase;
			r = Ldd.FreeFromAddr(gZoneUtilArray[index2].iAllocFixed, addr);
			}		

		gTestThread.Close();	
		}
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);		
	TestEnd();


	test.Next(_L("Test6: Call function TRamDefragRequest::ClaimRamZone on specifc zone and at the same time allocate pages to the zone"));	
	TestStart();		
	
	TInt pagesAlloc = 1; // Try and allocate just one page whilst attempting to claim the zone

	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		GetAllPageInfo();
		defragZoneID = gZoneConfigArray[index].iZoneId;
		TInt numPassed = 0;
		for (waitTime = 1000; waitTime > 0; waitTime-=10)
			{
			r = gTestThread.Create(gTestThreadName,MultiClaimZoneThreadFunc,KDefaultStackSize,0x1000,0x1000,(TAny*)defragZoneID);
			if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}

			gTestThread.Logon(status);
			gTestThread.Resume();
			test(status.Int() == KRequestPending);
			User::After(waitTime);
			
			r = Ldd.ZoneAllocDiscontiguous(defragZoneID, pagesAlloc);

			TESTDEBUG(test.Printf(_L("r = %d\n"), r));
			
			User::WaitForRequest(status);
			r2 = status.Int();
				
			GetAllPageInfo();
			TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
			if (freeInOthers && r2 != KErrNone)
				{
				test.Printf(_L("Claim was unsuccsessful: r2 = %d, expected = %d\n"), r2, KErrNone);
				}
			else if (r2 == KErrNone && r != KErrNoMemory)
				{
				test.Printf(_L("Fail: r = %d, expected = %d, r2 = %d, expected = %d, zone ID = 0x%x\n"), 
										r, KErrNoMemory, r2, KErrNone, defragZoneID);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(Ldd.FreeAllFixedPages());
				// Free from address as a different channel claimed the RAM zone.
				addr = gZoneConfigArray[index].iPhysBase;
				CLEANUP(Ldd.FreeFromAddr(gZoneUtilArray[index].iAllocFixed, addr));
				TEST_FAIL;
				}
			else
				{
				numPassed ++;
				}
			
			Ldd.FreeAllFixedPages();
		
			GetAllPageInfo();
			if (r2 == KErrNone)
				{// Free from address as a different channel claimed the RAM zone.
				addr = gZoneConfigArray[index].iPhysBase;
				r = Ldd.FreeFromAddr(gZoneUtilArray[index].iAllocFixed, addr);
				}
			gTestThread.Close();
			}
		if (numPassed > 0)
			{
			test.Printf(_L("Passed...\n"));	
			}	
		}	
	
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();



	test.Next(_L("Test7: Call function TRamDefragRequest::ClaimRamZone on a zone that contains fixed and movable pages"));
	TestStart();
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0) &&
			(gZoneUtilArray[index].iAllocMovable < 10 || gZoneUtilArray[index].iFreePages < 3))
			{
			-- index;
			}

	if (index == 0 && ((gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0) &&
			(gZoneUtilArray[index].iAllocMovable < 10 || gZoneUtilArray[index].iFreePages < 3)))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		defragZoneID = gZoneConfigArray[index].iZoneId;
		TESTDEBUG(test.Printf(_L("defragZoneID = 0x%x\n"), defragZoneID));		
		TESTDEBUG(test.Printf(_L("movable = 0x%x discardable = 0x%x\n"), 
						gZoneUtilArray[index].iAllocMovable, gZoneUtilArray[index].iAllocDiscardable));	

		
		Ldd.ZoneAllocDiscontiguous(defragZoneID, 2); // Allocated 2 fixed pages to ensure that zone cannot be emptied
		
		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, defragZoneID);
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (r != KErrNoMemory || 
			(freeInOthers && 
			(gZoneUtilArray[index].iAllocMovable || 
			gZoneUtilArray[index].iAllocDiscardable)))
			{
			test.Printf(_L("r = %d, expected = %d, or all movable/discardable pages have not been moved"), 
							r, KErrNoMemory);
			CLEANUP(Ldd.FreeAllFixedPages());
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));
			}
		
		Ldd.FreeAllFixedPages();
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();

	test.Next(_L("Test8: Call function TRamDefragRequest::ClaimRamZone on every zone one after the other"));
	TestStart();
		
	Ldd.ResetDriver();
			
	for (index = 0; index < gZoneCount; index ++)
		{
		// Variables for DP ROM cache sizes
		TUint minCacheSize = 0;
		TUint maxCacheSize = 0;
		TUint currentCacheSize = 0;

		if (gPagedRom)
			{
			DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
			TESTDEBUG(test.Printf(_L("Original CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
										minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
			// Calculate the maximum size of the DP cache 
			TUint minZoneFree = KMaxTUint;
			TUint maxDisFillBytes = 0;
			GetAllPageInfo();
			for (TUint i = 0; i < gZoneCount; i++)
				{
				maxDisFillBytes += gZoneUtilArray[i].iFreePages;
				maxDisFillBytes += gZoneUtilArray[i].iAllocDiscardable;
				if (minZoneFree > gZoneUtilArray[i].iFreePages)
					minZoneFree = gZoneUtilArray[i].iFreePages;
				}
			test.Printf(_L("Free pages 0x%x maxDisFillBytes 0x%x\n"), gTotalPageCount.iFreePages, maxDisFillBytes);
			maxDisFillBytes <<= gPageShift;

			r = DPTest::SetCacheSize(maxDisFillBytes, maxDisFillBytes);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			
			DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
			TESTDEBUG(test.Printf(_L("CacheSize2: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
										minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
			if (currentCacheSize != maxDisFillBytes)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			
			GetAllPageInfo();
			test.Printf(_L("Free pages after alloc discardable1 = 0x%x\n"), gTotalPageCount.iFreePages);

			r = DPTest::SetCacheSize(minZoneFree << gPageShift, maxDisFillBytes);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			
			// Check all zones have no free pages.
			GetAllPageInfo();
			for (TUint i = 0; i < gZoneCount; i++)
				{
				if (gZoneUtilArray[i].iFreePages != 0)
					{
					test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
					CLEANUP(ResetDPCache());
					TEST_FAIL;
					}
				}
			}
		else
			{
			// Fragment the RAM with some movable pages
			AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable);
			FreeMovable(gChunkArray1, gChunkArraySize1);
			}
		
		TUint zoneID = gZoneConfigArray[index].iZoneId;
		
		GetAllPageInfo();
		TUint origUnknownInZone = gZoneUtilArray[index].iAllocUnknown;
		TUint origMovInZone = gZoneUtilArray[index].iAllocMovable;
		TUint origFreeInZone = gZoneUtilArray[index].iFreePages;

		test.Printf(_L("Zone ID 0x%x - fixedPages = 0x%x, unknownPages = 0x%x, discardablePages = 0x%x movable = 0x%x free = 0x%x\n"), 
						zoneID, gZoneUtilArray[index].iAllocFixed, origUnknownInZone, gZoneUtilArray[index].iAllocDiscardable, 
						origMovInZone, origFreeInZone);


		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, zoneID);

		GetAllPageInfo();		
		TUint unknownPages = gZoneUtilArray[index].iAllocUnknown;
		TUint discPages = gZoneUtilArray[index].iAllocDiscardable;
		TUint movablePages = gZoneUtilArray[index].iAllocMovable;
	
		if (r == KErrNone)
			{
			r = Ldd.FreeAllFixedPages();
			}
		TUint freeInOtherZones = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (!unknownPages && movablePages && freeInOtherZones)
			{
			test.Printf(_L("Fail: Zone ID %x all the movable pages haven't been moved\n"), zoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}

		if (origUnknownInZone != unknownPages)
			{
			test.Printf(_L("Fail: Zone ID %x unknown pages before and after are not equal\n"), zoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}

		if (!gPagedRom)
			{
			if (!unknownPages && freeInOtherZones && discPages)
				{
				test.Printf(_L("Fail: Zone ID %x all the discardable pages haven't been moved\n"), zoneID);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			}
		else
			{
			DPTest::CacheSize(minCacheSize, maxCacheSize, currentCacheSize);
			TESTDEBUG(test.Printf(_L("CacheSize3: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
										minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
			// The the discardable pages should have been discarded or moved unless 
			// there is no room in the other zones and the cache size is already at its minimum
			if (!unknownPages && discPages && (freeInOtherZones || currentCacheSize != minCacheSize))
				{
				test.Printf(_L("Fail: Zone ID 0x%x all the discardable pages haven't been moved\n"), zoneID);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			}
		test.Printf(_L("Passed...\n"));
		}
	// TestEnd() will cleanup what's required.
	TestEnd();
	test.End();
	return 0;
	}


//
// TestCancelDefrag
//
//-----------------------------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0533
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the implementation of the function TRamDefragRequest::CancelDefrag()
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Following this, start a general defrag and cancel it
//! 	2.	Fragment the memory. Following this, start a zone defrag and cancel it 
//! 	3.	Fragment the memory. Following this, start a claim zone and cancel it
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrCancel
//! 	2.	KErrCancel
//! 	3.	KErrCancel
//-----------------------------------------------------------------------------------------------------------------------------------------
TInt TestCancelDefrag()
	{
	TInt r = 0;
	TUint defragZoneID = 0;

	test.Start(_L("Test1: Call general defrag and cancel it"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	r = Ldd.CheckCancel(DEFRAG_TYPE_GEN);

	TESTDEBUG(test.Printf(_L("r = %d\n"), r));
	if (r != KErrCancel)
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrCancel);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test2: Call zone defrag and cancel it"));	
	TestStart();		
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && 
			(gZoneUtilArray[index].iFreePages == gZoneUtilArray[index].iPhysPages ||
			gZoneUtilArray[index].iAllocFixed !=0 ||
			gZoneUtilArray[index].iAllocUnknown !=0))
		{	
		-- index; 
		}
	if (index == 0 && 
		(gZoneUtilArray[index].iFreePages == gZoneUtilArray[index].iPhysPages ||
		gZoneUtilArray[index].iAllocFixed !=0 ||
		gZoneUtilArray[index].iAllocUnknown !=0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{		
		defragZoneID = gZoneConfigArray[index].iZoneId;
		TESTDEBUG(test.Printf(_L("defragZoneID = 0x%x\n"),defragZoneID));

		r = Ldd.CheckCancel(DEFRAG_TYPE_EMPTY, defragZoneID);
		
		TESTDEBUG(test.Printf(_L("r = %d\n"), r));
		if (r != KErrCancel)
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrCancel);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}	
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}	

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test3: Call Claim RAM Zone and cancel it"));	
	TestStart();
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;	
	while (index > 0 && 
			(gZoneUtilArray[index].iFreePages == gZoneUtilArray[index].iPhysPages ||
			gZoneUtilArray[index].iAllocFixed !=0 ||
			gZoneUtilArray[index].iAllocUnknown !=0))
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;
	TESTDEBUG(test.Printf(_L("defragZoneID = 0x%x\n"),defragZoneID));

	if (index == 0 && 
			(gZoneUtilArray[index].iFreePages == gZoneUtilArray[index].iPhysPages ||
			gZoneUtilArray[index].iAllocFixed !=0 ||
			gZoneUtilArray[index].iAllocUnknown !=0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = Ldd.CheckCancel(DEFRAG_TYPE_CLAIM, defragZoneID);

		TESTDEBUG(test.Printf(_L("r = %d\n"), r));
		if (r != KErrCancel)
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrCancel);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}	
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd(); 

	test.End();
	return 0;
	}

//
// TestZoneAllocNoAffect
//
// Ensures that fixed page allocations do not affect the movable
// or discardable page allocations
//
TInt TestZoneAllocNoAffect(TInt aZoneAllocType, TInt aPageType)
	{
	TInt retVal = KErrNone;
	TInt r = KErrNone;
	TInt mostPrefIndex = 0;
	TUint leastPrefIndex = 0;
	TUint leastPrefZoneID = 0;

	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	switch(aPageType)
		{
		case BEST_MOVABLE:
			test.Printf(_L("Test Epoc::ZoneAllocPhysicalRam() doesn't affect the allocation of movable pages\n"));
			break;

		case BEST_DISCARDABLE:
			test.Printf(_L("Test Epoc::ZoneAllocPhysicalRam() doesn't affect the allocation of discardable pages\n"));
			if (!gPagedRom)
				{
				test.Printf(_L("Not a paged ROM - Skipping...\n"));
				goto skipSetup;
				}
			break;
		}
	// Fist find the zone that movable page allocations should go into
	// Getting the best movable will be the same for discardable as well
	mostPrefIndex = GetBestZone(aPageType);
	if (mostPrefIndex == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		goto skipSetup;
		}

	// Get the least pref zone to zone alloc into and ensure it has free pages
	GetPrefOrder();
	leastPrefIndex = gPrefArray[gZoneCount - 1];
	if (gZoneConfigArray[mostPrefIndex].iPref == gZoneConfigArray[leastPrefIndex].iPref)
		{
		test.Printf(_L("Zones with same preference - Skipping...\n"));
		goto skipSetup;
		}
	if (leastPrefIndex == (TUint)mostPrefIndex ||
		gZoneUtilArray[leastPrefIndex].iFreePages == 0)
		{
		test.Printf(_L("leastPrefIndex = mostPrefIndex or leastPrefIndex(%d) / mostPrefIndex(%d) has 0 free - Skipping test step...\n"), 
						leastPrefIndex, mostPrefIndex);
		goto skipSetup;
		}


	// Zone alloc 1 fixed page into the least preferable zone
	leastPrefZoneID = gZoneConfigArray[leastPrefIndex].iZoneId;	
	GetOriginalPageCount();
	switch(aZoneAllocType)
		{
		case Z_ALLOC_DISC:
			{
			r = Ldd.ZoneAllocDiscontiguous(leastPrefZoneID, 1);
			break;
			}

		case Z_ALLOC_CONTIG:
			{			
			r = Ldd.ZoneAllocContiguous(leastPrefZoneID, gPageSize);
			break;
			}
		}
	GetAllPageInfo();
	if (r != KErrNone ||
		gZoneUtilArray[leastPrefIndex].iAllocFixed <= gOriginalPageCountArray[leastPrefIndex].iAllocFixed)
		{
		test.Printf(_L("Failed to allocate 1 fixed page to zone index %d r = %d\n"), leastPrefIndex, r);
		goto skipSetup;
		}
	test.Printf(_L("leastPrefIndex = %d mostPrefIndex = %d\n"), leastPrefIndex, mostPrefIndex);
	switch(aPageType)
		{
		case BEST_MOVABLE:
			// now allocate 1 movable page
			r = AllocMovable(gChunkArray1, gChunkArraySize1, 1, gPageSize);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to allocate 1 movable page r = %d\n"), r);
				goto skipSetup;
				}
			break;
			
		case BEST_DISCARDABLE:
			// now allocate 1 discardable page
			TInt disPages;					
			TUint disBytes = (gTotalPageCount.iDiscardablePages + 1) << gPageShift;
			r = AllocDiscardable(disPages, disBytes);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to allocate 1 discardable page r = %d\n"), r);
				goto skipSetup;
				}
			break;
		}
	
	GetAllPageInfo();
	switch(aPageType)
		{
		case BEST_MOVABLE:
			if (gOriginalPageCountArray[mostPrefIndex].iAllocMovable >= gZoneUtilArray[mostPrefIndex].iAllocMovable ||
				gOriginalPageCountArray[leastPrefIndex].iAllocMovable < gZoneUtilArray[leastPrefIndex].iAllocMovable)
				{
				test.Printf(_L("FAIL: mostPrefIndex(%d) origMov 0x%x curMov 0x%x leastPrefIndex(%d) origMov 0x%x curMov 0x%x\n"), 
								mostPrefIndex, gOriginalPageCountArray[mostPrefIndex].iAllocMovable, gZoneUtilArray[mostPrefIndex].iAllocMovable, 
								leastPrefIndex, gOriginalPageCountArray[leastPrefIndex].iAllocMovable, gZoneUtilArray[leastPrefIndex].iAllocMovable);
				CLEANUP(Ldd.FreeAllFixedPages());
				retVal = KErrGeneral;
				}
			else
				{
				test.Printf(_L("Passed...\n"));
				}
			break;

			
		case BEST_DISCARDABLE:
			if (gOriginalPageCountArray[mostPrefIndex].iAllocDiscardable >= gZoneUtilArray[mostPrefIndex].iAllocDiscardable ||
				gOriginalPageCountArray[leastPrefIndex].iAllocDiscardable < gZoneUtilArray[leastPrefIndex].iAllocDiscardable)
				{
				test.Printf(_L("FAIL: mostPrefIndex(%d) origDis 0x%x curDis 0x%x leastPrefIndex(%d) origDis 0x%x curDis 0x%x\n"), 
								mostPrefIndex, gOriginalPageCountArray[mostPrefIndex].iAllocDiscardable, gZoneUtilArray[mostPrefIndex].iAllocDiscardable, 
								leastPrefIndex, gOriginalPageCountArray[leastPrefIndex].iAllocDiscardable, gZoneUtilArray[leastPrefIndex].iAllocDiscardable);
				CLEANUP(Ldd.FreeAllFixedPages());
				retVal = KErrGeneral;
				}
			else
				{
				test.Printf(_L("Passed...\n"));
				}
			break;
		}
	
	// This will clean up any fixed pages allocated.
skipSetup:
	TestEnd();
	return retVal;
	}
		
//
// TestZoneAllocContiguous
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0535
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the contiguous overload of Epoc::ZoneAllocPhysicalRam().
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Call function with a valid aZoneID.  
//! 	2.	Call function with an invalid aZoneID
//! 	3.	Call function with aSize > zone size 
//!		4.	Call multiple RAM zone overload of the function with a mix of valid and invalid IDs.
//!		5.	Call multiple RAM zone overload of the function with contiguous RAM zones and attempt
//!			to allocate over both RAM zones.
//!		6.	Call function on a RAM zone that has some non-DP pages allocated into it and ask
//!			for the whole RAM zone to be filled with fixed pages.
//!		7.	Get the most preferable zone for movable page allocations (mostPref). Allocate 1 contiguous fixed page 
//!			in the least preferable zone. Following this allocate 1 movable page
//!		8.	Get the most preferable zone for discardable page allocations (mostPref). Allocate 1 contiguous fixed 
//!			page in the least preferable zone. Following this allocate 1 discardable page
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrNone
//! 	2.	KErrArgument 
//! 	3.	KErrArgument
//!		4.	KErrArgument
//!		5.	KErrNone
//!		6.	KErrNoMemory
//!		7.	Movable pages are allocated into zone mostPref.
//!		8.	Discardable pages are allocated into zone mostPref
//---------------------------------------------------------------------------------------------------------------------
TInt TestZoneAllocContiguous()
	{
	TInt r = 0;
	test.Start(_L("Test1: Call function Epoc::ZoneAllocPhysicalRam() with a valid aZoneID"));	
	TestStart();	
		
	const TUint KAllocPages = 2;
	
	GetOriginalPageCount();
	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && gZoneUtilArray[index].iFreePages < KAllocPages)
		{
		-- index;
		}
	TUint zoneID = gZoneConfigArray[index].iZoneId;
	if (index == 0 && gZoneUtilArray[index].iFreePages < KAllocPages)
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		TUint allocBytes = KAllocPages << gPageShift;
		r = Ldd.ZoneAllocContiguous(zoneID, allocBytes);
		GetAllPageInfo();
		if (r == KErrNone && 
			gZoneUtilArray[index].iAllocFixed - gOriginalPageCountArray[index].iAllocFixed == KAllocPages)
			{
			test.Printf(_L("Pass: Correct number of fixed pages allocated to zone ID 0x%x\n"),zoneID);
			}
		else
			{
			test.Printf(_L("Fail: r = %d, expected = %d, or number of pages allocated is not expected\n"), 
							r, KErrNone);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}

		Ldd.FreeAllFixedPages();
		}
	TestEnd();


	test.Next(_L("Test2: Call function Epoc::ZoneAllocPhysicalRam() with an invalid aZoneID"));	
	TestStart();	
	
	zoneID = KInvalidZoneID; 
 
	TESTDEBUG(test.Printf(_L("zoneID = 0x%x\n"), zoneID));
	
	r = Ldd.ZoneAllocContiguous(zoneID, gPageSize);
	GetAllPageInfo();
	if (r == KErrArgument)
		{
		test.Printf(_L("Pass: Correct return value\n"));
		}
	else
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	Ldd.FreeAllFixedPages();
	TestEnd();


	test.Next(_L("Test3: Call function Epoc::ZoneAllocPhysicalRam() with aSize > zone size"));	
	TestStart();	
	
	index = gZoneCount - 1;
	zoneID = gZoneConfigArray[index].iZoneId;

	TUint allocBytes = (gZoneUtilArray[index].iPhysPages + 5)  << gPageShift;
	
	r = Ldd.ZoneAllocContiguous(zoneID, allocBytes);
	GetAllPageInfo();
	if (r == KErrArgument)
		{
		test.Printf(_L("Pass: Correct return value and number of pages allocated is correct\n"));
		}
	else
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	Ldd.FreeAllFixedPages();
	TestEnd();

	const TUint KMultiZoneIds = 10;
	TUint* zoneIdArray = new TUint[KMultiZoneIds];
	test_NotNull(zoneIdArray);

	test.Next(_L("Test4: Test Epoc::ZoneAllocPhysicaRam() always fails when at least one ID is invalid"));
	TestStart();

	index = gZoneCount - 1;
	while (index > 0 && gZoneUtilArray[index].iFreePages < KAllocPages)
		{
		-- index;
		}
	if (index == 0 && gZoneUtilArray[index].iFreePages < KAllocPages)
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		TUint zoneIdSize = 2;
		zoneIdArray[0] = gZoneConfigArray[index].iZoneId;
		zoneIdArray[1] = KInvalidZoneID;
		TUint allocBytes = KAllocPages << gPageShift;
		r = Ldd.MultiZoneAllocContiguous(zoneIdArray, zoneIdSize, allocBytes);
		if (r != KErrArgument)
			{
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Pass: Correct return value\n"));
			}
		}

	TestEnd();

	test.Next(_L("Test5: Test Epoc::ZoneAllocPhysicalRam() can span multiple RAM zones"));
	TestStart();
	// Attempt to find to physically contiguous RAM zones where higher addressed
	// one is empty, relies on RAM zones are returned by HAL functions in 
	// ascending physical address order.
	GetAllPageInfo();
	TBool zonesFound = EFalse;
	index = gZoneCount - 1;
	for (; index > 1; index--)
		{
		if (gZoneUtilArray[index].iFreePages == gZoneUtilArray[index].iPhysPages &&
			gZoneUtilArray[index-1].iFreePages == gZoneUtilArray[index-1].iPhysPages &&
			gZoneConfigArray[index].iPhysBase - 1 == gZoneConfigArray[index-1].iPhysEnd)
			{
			zonesFound = ETrue;
			break;
			}
		}

	if (!zonesFound)
		{
		test.Printf(_L("Cannot find RAM zones to perform test, Skipping test step...\n"));
		}
	else
		{
		// Allocate one page more than the first RAM zone to force the allocation
		// to spread over both RAM zones.
		TUint allocPages = gZoneUtilArray[index-1].iPhysPages + 1;
		TUint allocBytes = allocPages << gPageShift;

		// Attempt to find a zone that has less free pages than the allocation
		// size so we can test that the method continues past too full RAM zones.
		zonesFound = EFalse;
		TUint noAllocZone = 0;
		for (; noAllocZone < gZoneCount; noAllocZone++)
			{
			if (allocPages > gZoneUtilArray[noAllocZone].iFreePages &&
				noAllocZone != index && noAllocZone != index-1)
				{
				zonesFound = ETrue;
				break;
				} 
			}
		TUint zoneIds = 2;
		if (!zonesFound)
			{
			zoneIdArray[0] = gZoneConfigArray[index-1].iZoneId;
			zoneIdArray[1] = gZoneConfigArray[index].iZoneId;
			}
		else
			{// Have a zone that won't meet the allocation so use it
			TESTDEBUG(test.Printf(_L("noAllocZone ID %x\n"), gZoneConfigArray[noAllocZone].iZoneId));
			zoneIds++;
			zoneIdArray[0] = gZoneConfigArray[noAllocZone].iZoneId;
			zoneIdArray[1] = gZoneConfigArray[index-1].iZoneId;
			zoneIdArray[2] = gZoneConfigArray[index].iZoneId;
			}

		r = Ldd.MultiZoneAllocContiguous(zoneIdArray, zoneIds, allocBytes);

		GetAllPageInfo();
		if (r != KErrNone || 
			gZoneUtilArray[index].iFreePages == gZoneUtilArray[index].iPhysPages ||
			gZoneUtilArray[index-1].iFreePages == gZoneUtilArray[index-1].iPhysPages)
			{// The allocation failed.
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));
			}
		}
	// TestEnd() will free the allocation of fixed pages.
	TestEnd();

	test.Next(_L("Test6: Test Epoc::ZoneAllocPhysicalRam() returns KErrNoMemory when appropriate"));
	TestStart();
	// Attempt to find a RAM zone with some non-discarable pages allocated into it.
	// (At time of writing discardable pages wouldn't be discarded on demand by 
	// this function but this may be changed in the future as discontiguous case does that).
	GetAllPageInfo();
	zonesFound = EFalse;
	index = gZoneCount - 1;
	for (; index > 0; index--)
		{
		if (gZoneUtilArray[index].iFreePages && 
			(gZoneUtilArray[index].iAllocMovable || gZoneUtilArray[index].iAllocFixed))
			{
			zonesFound = ETrue;
			break;
			}
		}
	
	if (!zonesFound)
		{
		test.Printf(_L("Cannot find RAM zones to perform test, Skipping test step...\n"));		
		}
	else
		{
		// Attempt to allocate the whole of the RAM zone.
		GetOriginalPageCount();
		TUint allocBytes = gZoneConfigArray[index].iPhysPages << gPageShift;
		r = Ldd.ZoneAllocContiguous(gZoneConfigArray[index].iZoneId, allocBytes);

		// The allocation should have failed and no pages should have
		// been allocated.
		GetAllPageInfo();
		if (r != KErrNoMemory ||
			gOriginalPageCountArray[index].iFreePages > gZoneUtilArray[index].iFreePages)
			{
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Pass: Correct return value\n"));
			}
		}
	// TestEnd() will free the allocation of fixed pages.
	TestEnd();
	delete[] zoneIdArray;

	
	test.Next(_L("Test7: Test Epoc::ZoneAllocPhysicalRam() (Contiguous) doesn't affect the allocation of movable pages"));
	r = TestZoneAllocNoAffect(Z_ALLOC_CONTIG, BEST_MOVABLE);
	if (r != KErrNone)
		{
		TEST_FAIL;
		}

	test.Next(_L("Test8: Test Epoc::ZoneAllocPhysicalRam() (Contiguous) doesn't affect the allocation of discardable pages"));
	r = TestZoneAllocNoAffect(Z_ALLOC_CONTIG, BEST_DISCARDABLE);
	if (r != KErrNone)
		{
		TEST_FAIL;
		}

	test.End();
	return KErrNone;
	}


//
// TestZoneAllocDiscontiguous
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0536
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the discontiguous overload of Epoc::ZoneAllocPhysicalRam().
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Call function with a valid aZoneID.  
//! 	2.	Call function with an invalid aZoneID
//! 	3.	Call function with aNumPages > zone size 
//!		4.	Call multiple RAM zone overload of function with a mix of valid and invalid IDs
//!		5.	Call multiple RAM zone overload of function so that the allocation will have to
//!			span multiple RAM zones.
//!		6.	Call function with memory full with DP cache that has reached it's 
//!			minimum cache size.
//!		7.	Call function with memory not quite full with DP cache that has reached it's 
//!			minimum cache size and with the specified RAM zone full.
//!		8.	Call function to allocate a whole RAM zone on a RAM zone that has non-discardable
//!			pages already allocated into it.
//!		9.	Call function to allocate one less than the whole RAM zone on a RAM zone that has movable 
//!			pages allocated.
//!		10.	Get the most preferable zone for movable page allocations (mostPref). Allocate 1 discontiguous fixed page 
//!			in the least preferable zone. Following this allocate 1 movable page
//!		11.	Get the most preferable zone for discardable page allocations (mostPref). Allocate 1 discontiguous fixed 
//!			page in the least preferable zone. Following this allocate 1 discardable page
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrNone
//! 	2.	KErrArgument
//! 	3.	KErrArgument
//!		4.	KErrArgument
//!		5.	KErrNone
//!		6.	KErrNoMemory
//!		7.	KErrNone
//!		8. 	KErrNoMemory
//!		9.	KErrNone (i.e. the movable pages are shifted out of the way but only those that need to be moved).
//!		10.	Movable pages are allocated into zone mostPref.
//!		11.	Discardable pages are allocated into zone mostPref
//---------------------------------------------------------------------------------------------------------------------
TInt TestZoneAllocDiscontiguous()
	{
	TInt r = KErrNone;
	test.Start(_L("Test1: Call function Epoc::ZoneAllocPhysicalRam() with a valid aZoneID"));	
	TestStart();	
	
	const TUint KAllocPages = 5;
	// Detemine how many extra pages the kernel heap may grow by
	// as these may need to be accounted for.
	TUint fixedOverhead = Ldd.GetAllocDiff(KAllocPages);

	GetOriginalPageCount();
	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && gZoneUtilArray[index].iFreePages < KAllocPages)
		{
		-- index;
		}

	if (gZoneUtilArray[index].iFreePages < KAllocPages || gTotalPageCount.iFreePages < KAllocPages + fixedOverhead)
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		// Allocate KAllocPages discontiguous fixed pages into RAM zone zoneID 
		// and verfiy that the pages were allocated to the correct zone, allow for
		// extra fixed pages to be allocated as the kernel heap may grow.
		TUint zoneID = gZoneConfigArray[index].iZoneId;
		r = Ldd.ZoneAllocDiscontiguous(zoneID, KAllocPages);

		GetAllPageInfo();
		if ((r == KErrNone) && 
			(gZoneUtilArray[index].iAllocFixed >= gOriginalPageCountArray[index].iAllocFixed + KAllocPages))
			{
			test.Printf(_L("Pass: Correct return value and number of pages allocated is correct\n"));
			}
		else
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		Ldd.FreeAllFixedPages();
		}	
	TestEnd();


	test.Next(_L("Test2: Call function Epoc::ZoneAllocPhysicalRam() with an invalid aZoneID"));	
	TestStart();	
	
	TUint zoneID = KInvalidZoneID;
	
	r = Ldd.ZoneAllocDiscontiguous(zoneID, KAllocPages);
	GetAllPageInfo();
	if (r == KErrArgument)
		{
		test.Printf(_L("Pass: Correct return value\n"));
		}
	else
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	Ldd.FreeAllFixedPages();
	TestEnd();

	test.Next(_L("Test3: Call function Epoc::ZoneAllocPhysicalRam() when aNumPages > zone size"));	
	TestStart();	
	
	GetAllPageInfo();
	index = gZoneCount - 1;
	zoneID = gZoneConfigArray[index].iZoneId;
	
	TUint allocPages = gZoneUtilArray[index].iPhysPages + 1;
	r = Ldd.ZoneAllocDiscontiguous(zoneID, allocPages);
	
	GetAllPageInfo();
	if (r == KErrArgument)
		{
		test.Printf(_L("Pass: Correct return value and number of pages allocated is correct\n"));
		}
	else
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrArgument);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}
	Ldd.FreeAllFixedPages();
	TestEnd();

	const TUint KMultiZoneIds = 10;
	TUint* zoneIdArray = new TUint[KMultiZoneIds];
	test_NotNull(zoneIdArray);

	test.Next(_L("Test4: Test Epoc::ZoneAllocPhysicaRam() always fails when at least one ID is invalid"));
	TestStart();

	TBool zonesFound = EFalse;
	index = gZoneCount - 1;
	for (; index > 0; index--)
		{
		if (gZoneUtilArray[index].iFreePages >= KAllocPages)
			{
			zonesFound = ETrue;
			break;
			}
		}
	if (!zonesFound)
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		TUint zoneIdSize = 2;
		zoneIdArray[0] = gZoneConfigArray[index].iZoneId;
		zoneIdArray[1] = KInvalidZoneID;
		r = Ldd.MultiZoneAllocDiscontiguous(zoneIdArray, zoneIdSize, KAllocPages);
		if (r != KErrArgument)
			{// Make sure we cleanup.
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}

	TestEnd();

	test.Next(_L("Test5: Test Epoc::ZoneAllocPhysicalRam() can span multiple RAM zones"));
	TestStart();
	zonesFound = EFalse;
	TUint zonesCount = 0;
	const TUint KTest5Zones = 2;
	TUint zoneIndices[KTest5Zones];
	allocPages = 0;

	// Attempt to find KTest5Zones RAM zones with some free pages,
	// search in reverse preference order to reduce chances of kernel heap pages
	// being allocated into the RAM zones under test.
	GetPrefOrder();
	index = gZoneCount - 1;
	for (; index > 0; index--)
		{
		TUint prefIndex = gPrefArray[index];
		if (gZoneUtilArray[prefIndex].iFreePages != 0)
			{
			allocPages += gZoneUtilArray[prefIndex].iFreePages;
			zoneIndices[zonesCount++] = prefIndex;
			if (zonesCount == KTest5Zones)
				{
				zonesFound = ETrue;
				break;
				}
			}
		}

	if (!zonesFound)
		{
		test.Printf(_L("Cannot find RAM zones to perform test, Skipping test step...\n"));
		}
	else
		{
		// Attempt to find a zone that has less free pages than the allocation
		// size so we can test that the method continues past too full RAM zones.
		zonesFound = EFalse;
		TUint noAllocZone = 0;
		for (; noAllocZone < gZoneCount; noAllocZone++)
			{
			if (allocPages > gZoneUtilArray[noAllocZone].iFreePages &&
				noAllocZone != zoneIndices[0] && noAllocZone != zoneIndices[1])
				{
				zonesFound = ETrue;
				break;
				}
			}
		
		if (gPagedRom)
			{// Fill memory with DP pages to test the allocation will discard
			// pages when necessary.
			TInt discard;
			r = AllocDiscardable(discard);
			GetAllPageInfo();
			if (r != KErrNone ||
				gZoneUtilArray[zoneIndices[0]].iFreePages != 0 || 
				gZoneUtilArray[zoneIndices[1]].iFreePages != 0)
				{
				test.Printf(_L("r %d\n"), r);
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}			
			}

		TUint zoneIds = KTest5Zones;
		if (!zonesFound)
			{
			zoneIdArray[0] = gZoneConfigArray[zoneIndices[0]].iZoneId;
			zoneIdArray[1] = gZoneConfigArray[zoneIndices[1]].iZoneId;
			}
		else
			{// Have a zone that won't meet the allocation so use it
			TESTDEBUG(test.Printf(_L("noAllocZone ID %x\n"), gZoneConfigArray[noAllocZone].iZoneId));
			zoneIds++;
			zoneIdArray[0] = gZoneConfigArray[noAllocZone].iZoneId;
			zoneIdArray[1] = gZoneConfigArray[zoneIndices[0]].iZoneId;
			zoneIdArray[2] = gZoneConfigArray[zoneIndices[1]].iZoneId;
			}

		// Adjust the allocation size for any kernel heap pages that may be
		// required as they may get allocated into the RAM zones under test.
		allocPages -= Ldd.GetAllocDiff(allocPages);

		GetOriginalPageCount();
		r = Ldd.MultiZoneAllocDiscontiguous(zoneIdArray, zoneIds, allocPages);

		GetAllPageInfo();
		if (r != KErrNone ||
			gZoneUtilArray[zoneIndices[0]].iAllocFixed <= gOriginalPageCountArray[zoneIndices[0]].iAllocFixed ||
			gZoneUtilArray[zoneIndices[1]].iAllocFixed <= gOriginalPageCountArray[zoneIndices[1]].iAllocFixed)
			{// The allocation failed.
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	// TestEnd() will free the allocation of fixed pages.
	TestEnd();


	if (gPagedRom)
		{
		test.Next(_L("Test6: Test Epoc::ZoneAllocPhysicalRam() returns KErrNoMemory when DP minimum cache size is hit"));
		TestStart();
		GetPrefOrder();
		TUint zoneFreePages = 0;
		index = gZoneCount - 1;
		for (; index > 0; index--)
			{
			TUint prefIndex = gPrefArray[index];
			zoneFreePages = gZoneUtilArray[prefIndex].iFreePages;
			if (zoneFreePages > 2)
				{
				index = prefIndex;
				zonesFound = ETrue;
				break;
				}
			}

		if (!zonesFound)
			{
			test.Printf(_L("Cannot find RAM zones to perform test, Skipping test step...\n"));
			}
		else
			{
			// Fill the RAM with DP cache pages and up the minimum cache size so
			// that the allocation will fail.
			TInt discardBytes;
			TInt r = AllocDiscardable(discardBytes, KMaxTUint64, (zoneFreePages - 1) << gPageShift);
			test_KErrNone(r);

			// Ensure that the RAM zone under test is full.
			GetAllPageInfo();
			if (gZoneUtilArray[index].iFreePages != 0)
				{
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}

			r = Ldd.ZoneAllocDiscontiguous(gZoneConfigArray[index].iZoneId, zoneFreePages);

			if (r != KErrNoMemory)
				{
				CLEANUP(Ldd.FreeAllFixedPages());
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));	
				}
			}
		TestEnd();

		test.Next(_L("Test7: Test Epoc::ZoneAllocPhysicalRam() replaces DP cache when DP minimum cache size is hit"));
		TestStart();
		gChunkArray1 = new RChunk;
		gChunkArraySize1 = 1;
		GetPrefOrder();
		zonesFound = EFalse;
		for (index = gZoneCount - 1; index > 0 && !zonesFound; index--)
			{
			TUint prefIndex = gPrefArray[index];
			zoneFreePages = gZoneUtilArray[prefIndex].iFreePages;
			if (zoneFreePages > 1)
				{
				// Check there is at least one free page in the other RAM zones.
				TUint i = 0;
				for (; i < gZoneCount; i++)
					{
					if (i != prefIndex && gZoneUtilArray[i].iFreePages != 0)
						{
						index = prefIndex;
						zonesFound = ETrue;
						break;
						}
					}
				}
			}
		if (!zonesFound)
			{
			test.Printf(_L("Cannot find RAM zones to perform test, Skipping test step...\n"));
			}
		else
			{
			index++;
			// Attempt to allocate a movable page to create a gap for the DP cache 
			// page to be replaced with.
			GetOriginalPageCount();
			r = AllocMovable(gChunkArray1, gChunkArraySize1, 1, gPageSize);

			GetAllPageInfo();
			TESTDEBUG(test.Printf(_L("index %d prev free 0x%x cur free 0x%x\n"), 
										index, gOriginalPageCountArray[index].iFreePages, gZoneUtilArray[index].iFreePages));

			if (r != KErrNone || 
				gOriginalPageCountArray[index].iFreePages != gZoneUtilArray[index].iFreePages)
				{// The gap was allocated into the RAM zone under test so can't continue as
				// the DP cache will attempt to be reallocated into the same RAM zone.
				test.Printf(_L("Cannot find RAM zones to perform test, Skipping test step...\n"));
				}
			else
				{
				// Get the ldd to create the array for the fixed page addresses here
				// so that any kernel heap allocations have already occurred before 
				// memory is filled etc.  Make allocation really large so it will always be enough.
				r = Ldd.AllocFixedArray(50);

				// Fill RAM with DP cache pages and free the gap.
				TInt discardBytes;
				r = AllocDiscardable(discardBytes, KMaxTUint64, 0);
				if (r != KErrNone)	
					{
					CLEANUP(ResetDPCache());
					TEST_FAIL;
					}
				UpdateRamInfo();
				TInt prevFreeBytes = gFreeRam;
				RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
				UpdateRamInfo();
				TInt freedPages = (gFreeRam - prevFreeBytes) >> gPageShift;
				if (freedPages < 1)
					{// Something went wrong as should have freed at least one page
					CLEANUP(ResetDPCache());
					TEST_FAIL;
					}
				TUint extraFreePages = freedPages - 1;

				// Ensure that the RAM zone under test is full.
				GetAllPageInfo();
				if (gZoneUtilArray[index].iFreePages != 0)
					{
					CLEANUP(ResetDPCache());
					TEST_FAIL;
					}

				// Allocate from the RAM zone which should force a DP cache 
				// page to be allocated.
				GetOriginalPageCount();
				TUint fixedAllocPages = 1 + extraFreePages;
				r = Ldd.ZoneAllocDiscontiguous2(gZoneConfigArray[index].iZoneId, fixedAllocPages);
			
				GetAllPageInfo();
				if (r != KErrNone ||
					gOriginalPageCountArray[index].iAllocFixed + fixedAllocPages != gZoneUtilArray[index].iAllocFixed)
					{
					test.Printf(_L("r %d index %d alloc 0x%x prevFixed 0x%x curFixed 0x%x\n"), r, index, fixedAllocPages, 
								gOriginalPageCountArray[index].iAllocFixed, gZoneUtilArray[index].iAllocFixed);
					CLEANUP(Ldd.FreeAllFixedPages());
					TEST_FAIL;
					}
				else
					{
					test.Printf(_L("Passed...\n"));						
					}
				}
			}
		// This should cleanup any fixed pages allocated.
		TestEnd();
		}

	test.Next(_L("Test8: Test Epoc::ZoneAllocPhysicalRam() return KErrNoMemory when appropriate"));
	TestStart();
	// Search for a RAM zone that has some immovable pages allocated into 
	// it but isn't totally full.
	GetAllPageInfo();
	zonesFound = EFalse;
	for (index = 0; index < gZoneCount; index++)
		{
		if (gZoneUtilArray[index].iFreePages && 
			(gZoneUtilArray[index].iAllocUnknown || gZoneUtilArray[index].iAllocFixed))
			{
			zonesFound = ETrue;
			break;
			}
		}
	if (!zonesFound)
		{
		test.Printf(_L("Cannot find RAM zones to perform test, Skipping test step...\n"));
		}
	else
		{
		// Attempt to allocate the whole RAM zone.
		r = Ldd.ZoneAllocDiscontiguous(gZoneConfigArray[index].iZoneId, gZoneUtilArray[index].iPhysPages);
		
		if (r != KErrNoMemory)
			{
			test.Printf(_L("FAIL: r %d index %d\n"), r, index);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));
			}
		}
	TestEnd();

	test.Next(_L("Test9: Test Epoc::ZoneAllocPhysicalRam() moves the required number of movable pages"));
	TestStart();
	TUint allocFixedPages;
	// Search for a RAM zone that has at least 2 movable pages allocated into it.
	// Need 2 so that we can move one and leave one.
	GetAllPageInfo();
	zonesFound = EFalse;
	for (index = 0; index < gZoneCount; index++)
		{
		if (gZoneUtilArray[index].iAllocMovable > 1)
			{
			// Only use this zone if the other RAM zones have enough free space for 
			// the movable page in this zone to be moved to.
			TUint freeInOther = 0;
			for (TUint i = 0; i < gZoneCount && !zonesFound; i++)
				{
				if (i != index)
					{
					freeInOther += gZoneUtilArray[i].iFreePages;
					}
				}
			if (freeInOther >= gZoneUtilArray[index].iAllocMovable)
				{
				zonesFound = ETrue;
				break;
				}
			}
		}
	
	if (!zonesFound)
		{
		test.Printf(_L("No suitable RAM zone could be found - Skipping...\n"));
		goto skipTest9;
		}

	// Allocate up to one less than the RAM zone size. Do 2 stage fixed allocation
	// to avoid kernel heap allocations spoiling test setup.
	r = Ldd.AllocFixedArray(gZoneConfigArray[index].iPhysPages);
	if (r != KErrNone)
		{
		test.Printf(_L("Not enough free RAM to perform test - Skipping...\n"));
		goto skipTest9;
		}

	GetAllPageInfo();
	if (gZoneUtilArray[index].iAllocMovable < 2)
		{
		test.Printf(_L("Expanding kernel heap for phys address array spoiled RAM zone - Skipping...\n"));
		goto skipTest9;
		}
	allocFixedPages =	gZoneUtilArray[index].iAllocMovable + 
						gZoneUtilArray[index].iAllocDiscardable +
						gZoneUtilArray[index].iFreePages - 1;
	r = Ldd.ZoneAllocDiscontiguous2(gZoneConfigArray[index].iZoneId, allocFixedPages);

	if (r != KErrNone || !gZoneUtilArray[index].iAllocMovable)
		{
		test.Printf(_L("Fixed not allocated or too many movable moved RAM zone ID%x\n"), 
					gZoneConfigArray[index].iZoneId);
		CLEANUP(Ldd.FreeAllFixedPages());
		TEST_FAIL;
		}

skipTest9 :
	// This will clean up any fixed pages allocated.
	TestEnd();

	delete[] zoneIdArray;
	
	test.Next(_L("Test10: Test Epoc::ZoneAllocPhysicalRam() (Discontiguous) doesn't affect the allocation of movable pages"));
	r = TestZoneAllocNoAffect(Z_ALLOC_DISC, BEST_MOVABLE);
	if (r != KErrNone)
		{
		TEST_FAIL;
		}

	test.Next(_L("Test11: Test Epoc::ZoneAllocPhysicalRam() (Discontiguous) doesn't affect the allocation of discardable pages"));
	r = TestZoneAllocNoAffect(Z_ALLOC_DISC, BEST_DISCARDABLE);
	if (r != KErrNone)
		{
		TEST_FAIL;
		}

	test.End();
	return KErrNone;
	}


//
// TestFreeZone
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0537
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the function Epoc::FreePhysicalRam()
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Allocate fixed pages and call function to free all fixed pages allocated.
//!		2.	Claim a RAM zone and then free it via Epoc::FreeRamZone().
//!		3.	Invoke Epoc::FreeRamZone() with an invalid RAM zone ID.
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrNone
//!		2.	KErrNone
//!		3.	KErrArgument
//---------------------------------------------------------------------------------------------------------------------
TInt TestFreeZone()
	{
	TInt r = 0;
	TUint zoneID = 0;
	test.Start(_L("Test1: Freeing allocated pages"));	
	TestStart();	
	
	TInt pages = 50;

	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && 
			(gZoneUtilArray[index].iAllocFixed != 0 || 
			gZoneUtilArray[index].iAllocUnknown != 0 || 
			(TInt)gZoneUtilArray[index].iFreePages < pages)) 
		{
		-- index;
		}
	zoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && 
		(gZoneUtilArray[index].iAllocFixed != 0 || 
		gZoneUtilArray[index].iAllocUnknown != 0 || 
		(TInt)gZoneUtilArray[index].iFreePages < pages))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		TESTDEBUG(test.Printf(_L("Allocating 0x%x pages to zone ID 0x%x\n"), pages, zoneID));
		r = Ldd.ZoneAllocDiscontiguous(zoneID, pages);
		GetAllPageInfo();

		TESTDEBUG(test.Printf(_L("Freeing 0x%x fixed pages\n"), pages));
		if (r == KErrNone)
			{
			r = Ldd.FreeZone(pages);
			}
		TESTDEBUG(test.Printf(_L("r = %d\n"), r));
		if (r == KErrNone)
			{
			test.Printf(_L("Pass: Correct return value\n"));
			}
		else
			{
			test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrNone);
			TEST_FAIL;
			}
		}
	TestEnd();
	test.End();

	test.Start(_L("Test2: Epoc::FreeRamZone() on a claimed RAM zone"));
	TestStart();
	GetAllPageInfo();
	TUint zoneIndex = 0;
	while (zoneIndex < gZoneCount)
		{
		if (gZoneUtilArray[zoneIndex].iFreePages == gZoneUtilArray[zoneIndex].iPhysPages)
			break;
		zoneIndex++;
		}
	if (zoneIndex >= gZoneCount)
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		goto Test2End;
		}
	zoneID = gZoneConfigArray[zoneIndex].iZoneId;
	r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, zoneID);
	if (r != KErrNone)
		{
		test.Printf(_L("Fail: r = %d, expected = %d\n"), r, KErrNone);
		TEST_FAIL;
		}
	GetAllPageInfo();
	if (gZoneUtilArray[zoneIndex].iPhysPages != gZoneUtilArray[zoneIndex].iAllocFixed)
		{
		test.Printf(_L("Fail: RAM zone ID %d not claimed successfully"), zoneID);
		TEST_FAIL;
		}
	r = Ldd.FreeZoneId(zoneID);
	GetAllPageInfo();
	if (r != KErrNone ||
		gZoneUtilArray[zoneIndex].iPhysPages != gZoneUtilArray[zoneIndex].iFreePages)
		{
		test.Printf(_L("Fail: RAM zone ID %d not freed successfully r=%d"), zoneID, r);
		TEST_FAIL;
		}
Test2End:
	TestEnd();
	test.End();

	test.Start(_L("Test2: Epoc::FreeRamZone() on an invalid RAM zone"));
	TestStart();
	r = Ldd.FreeZoneId(KInvalidZoneID);
	if (r != KErrArgument)
		{
		test.Printf(_L("Fail: Error RAM zone ID %d r=%d"), KInvalidZoneID, r);
		TEST_FAIL;
		}
	
	TestEnd();
	test.End();
	return KErrNone;
	}


//
// TestDefragSemMethod
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0538
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the semaphore versions of the various defrag methods
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Following this, call the semaphore variation of DefragRam.  
//! 	2.	Fragment the memory. Following this, call the semaphore variation of EmptyRamZone 
//! 	3.	Fragment the memory. Following this, call the semaphore variation of ClaimRamZone 
//! 
//! @SYMTestExpectedResults
//! 	1.	1 or more zones have been emptied
//! 	2.	Zone specified has been emptied
//! 	3.	Zone has been claimed
//---------------------------------------------------------------------------------------------------------------------
TInt TestDefragSemMethod()
	{
	TInt r = 0;
	
	test.Start(_L("Test1: Call semaphore method of DefragRam"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetOriginalPageCount();
	TBool genSucceed = CanGenSucceed();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SEM);
	
	
	if (r != KErrNone ||(genSucceed && CheckZonesSwitchedOff() == EFalse))
		{
		test.Printf(_L("Fail: r = %d, or zones have not been swtiched off\n"), r);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test2: Call semaphore method of EmptyRamZone"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	TUint defragZoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SEM, defragZoneID);
		
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && (r != KErrNone || !CheckZoneIsOff(index)))
			{
			test.Printf(_L("Fail: r = %d, or zones has not been swtiched off\n"), r);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	// This will free any allocated memory.			
	TestEnd();

	test.Next(_L("Test3: Call semaphore method of ClaimRamZone"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 2;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;
	
	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SEM, defragZoneID);

		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (CheckZoneIsOff(index) ||
			(freeInOthers && ( r != KErrNone ||
			gZoneUtilArray[index].iAllocFixed != gZoneConfigArray[index].iPhysPages)))
			{
			test.Printf(_L("Fail: r = %d, or zone ID 0x%x has not been claimed\n"), r, defragZoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}

		r = Ldd.FreeAllFixedPages();
		}

	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();

	test.End();
	return KErrNone;

	}


//
// TestDefragDfcMethod
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0539
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the Dfc versions of the various defrag methods
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory. Following this, call the Dfc variation of DefragRam.  
//! 	2.	Fragment the memory. Following this, call the Dfc variation of EmptyRamZone 
//! 	3.	Fragment the memory. Following this, call the Dfc variation of ClaimRamZone 
//! 
//! @SYMTestExpectedResults
//! 	1.	1 or more zones have been emptied
//! 	2.	Zone specified has been emptied
//! 	3.	Zone has been claimed
//---------------------------------------------------------------------------------------------------------------------
TInt TestDefragDfcMethod()
	{
	TInt r = 0;
	TRequestStatus req;
	test.Start(_L("Test1: Call Dfc method of DefragRam"));	
	TestStart();	
	
	GetAllPageInfo();
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetOriginalPageCount();
	TBool genSucceed = CanGenSucceed();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_DFC, 0, 0, -1, &req);
	TESTDEBUG(test.Printf(_L("After queueing defrag r = %d\n"), r));
	User::WaitForRequest(req);
	r = req.Int();
	
	if (r != KErrNone || (genSucceed && CheckZonesSwitchedOff() == EFalse))
		{
		test.Printf(_L("Fail: r = %d, or zones have not been swtiched off\n"), r);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test2: Call Dfc method of EmptyRamZone"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	TUint index = gZoneCount - 1;
	while (index > 0 && 
			(gZoneUtilArray[index].iAllocMovable < 10 ||
			gZoneUtilArray[index].iAllocFixed != 0 || 
			gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}

	TUint defragZoneID = gZoneConfigArray[index].iZoneId;
	test.Printf(_L("zone ID = 0x%x\n"), defragZoneID);
	if (index == 0 && 
		(gZoneUtilArray[index].iAllocMovable < 10 ||
		gZoneUtilArray[index].iAllocFixed != 0 || 
		gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_DFC, defragZoneID, 0, -1, &req);
		
		User::WaitForRequest(req);
		r = req.Int();

		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && (r != KErrNone || CheckZoneIsOff(index) == EFalse))
			{
			test.Printf(_L("Fail: r = %d, or zones have not been swtiched off\n"), r);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	// This will free any allocated memory
	TestEnd();
	

	test.Next(_L("Test3: Call Dfc method of ClaimRamZone"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	index = gZoneCount - 1;
	while (index > 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0)) 
		{
		-- index;
		}
	defragZoneID = gZoneConfigArray[index].iZoneId;

	if (index == 0 && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		test.Printf(_L("Cannot find zone to perform test, Skipping test step...\n"));
		}
	else
		{
		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_DFC, defragZoneID, 0, -1, &req);
		User::WaitForRequest(req);
		r = req.Int();
		
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (CheckZoneIsOff(index) || 
			(freeInOthers && (r != KErrNone || 
			gZoneUtilArray[index].iAllocFixed != gZoneConfigArray[index].iPhysPages)))
			{
			test.Printf(_L("Fail: r = %d, or zone ID 0x%x has not been claimed\n"), r, defragZoneID);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			if (r == KErrNone)
				{
				CLEANUP(Ldd.FreeFromAddr(	gZoneUtilArray[index].iAllocFixed, 
											gZoneConfigArray[index].iPhysBase));
				}
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		
		if (r == KErrNone)
			{
			Ldd.FreeFromAddr(	gZoneUtilArray[index].iAllocFixed, 
								gZoneConfigArray[index].iPhysBase);
			}
		}
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);		
	TestEnd();

	test.End();
	return KErrNone;
	}


//
// TestPriorities
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0540
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying the priorities of the defrag methods
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Call DefragRam with a lower invalid priority value, e.g. -2  
//! 	2.	Call DefragRam with a lower invalid priority value, e.g. 100
//! 	3.	Queue three asynchronous defrags using the Dfc method:
//! 			a.	First one with the lowest priority, this will start the straight away 
//! 				and will busy the defrag method, causing any other defrag requests to 
//! 				be queued. 
//! 			b.	Queue a defrag with a relatively low priority
//! 			c.	Queue a defrag with a higher priority than the one queued in (b)
//! 		Record the order in which the defrags are completed
//! 
//! @SYMTestExpectedResults
//! 	1.	KErrArgument
//! 	2.	KErrArgument
//! 	3.	(a) will complete first as it started straight away. 
//! 		(b) and (c) were both queued whilst (a) was running, 
//! 		however as (c) has a higher priority, it will complete first - 
//! 		therefore the order returned would be "a,c,b"
//---------------------------------------------------------------------------------------------------------------------
TInt TestPriorities()
	{
	test.Start(_L("Test1: Call defrag with an invalid lower priority"));	
	TestStart();	
	
	TInt r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, 0, -2);
	TESTDEBUG(test.Printf(_L("r = %d\n"), r));
	if (r != KErrArgument)
		{
		test.Printf(_L("Fail: r = %d, not expected\n"), r);
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	
	TestEnd();

	test.Next(_L("Test2: Call defrag with an invalid higher priority"));	
	TestStart();	
	
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC, 0, 0, 100);
	TESTDEBUG(test.Printf(_L("r = %d\n"), r));
	if (r != KErrArgument)
		{
		test.Printf(_L("Fail: r = %d, not expected\n"), r);
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));	
		}
	TestEnd();

	if (UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0) == 1)
		{// Only test priorities on single core system as this test is not smp safe.
		test.Next(_L("Test3: Call Dfc method of EmptyRamZone to test priorities"));	
		TestStart();
		
		
		TRequestStatus req;
		TRequestStatus req2;
		TRequestStatus req3;	
		TInt expectedOrder = 132; // Priorities set in Device driver 
		
		AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
		FreeMovable(gChunkArray1, gChunkArraySize1);

		TUint index = (gZoneCount + 1) / 2;
		TUint defragZoneID = gZoneConfigArray[index].iZoneId;

		r = Ldd.CheckPriorities(DEFRAG_TYPE_EMPTY, defragZoneID, &req, &req2, &req3);

		User::WaitForRequest(req);
		User::WaitForRequest(req2);
		User::WaitForRequest(req3);

		TInt order = Ldd.GetDefragOrder();
		if (order != expectedOrder)
			{
			test.Printf(_L("Fail: order = %d. expected = %d\n"), order, expectedOrder);
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		// This will free any allocated memory
		TestEnd();
		}

	test.End();
	return KErrNone;
	}


//
// TestFlags
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0541
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying that when certain flags are set, 
//! 							only certain types of pages can be allocated to the zone. 
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Set the NoFixed flag in a zone and allocate movable pages
//! 	2.	Set the NoFixed flag in a zone and allocate fixed pages
//! 	3.	Set the NoFixed flag in a zone and allocate discardable pages
//! 	4.	Set the NoFixed flag in a zone and allocate fixed pages to that zone using Epoc::ZoneAllocPhysicalRam()
//! 	5.	Set the NoFixed flag in a zone and allocate fixed pages by calling TRamDefragRequest::ClaimRamZone()
//! 	6.	Set the NoMovable flag in a zone and allocate movable pages
//! 	7.	Set the NoMovable flag in a zone and allocate fixed pages
//! 	8.	Set the NoMovable flag in a zone and allocate discardable pages
//! 	9.	Set the NoDiscardable flag in a zone and allocate movable pages
//! 	10.	Set the NoDiscardable flag in a zone and allocate fixed pages
//! 	11.	Set the NoDiscardable flag in a zone and allocate discardable pages
//! 	12.	Set the OnlyDiscardable flag in a zone and allocate movable pages
//! 	13.	Set the OnlyDiscardable flag in a zone and allocate fixed pages
//! 	14.	Set the OnlyDiscardable flag in a zone and allocate discardable pages
//! 	15.	Set the OnlyDiscardable flag in a zone and allocate fixed pages to that zone using Epoc::ZoneAllocPhysicalRam()
//! 	16.	Set the OnlyDiscardable flag in a zone and allocate fixed pages by calling TRamDefragRequest::ClaimRamZone()
//! 	17.	Set the NoFurtherAlloc flag in a zone and allocate movable pages
//! 	18.	Set the NoFurtherAlloc flag in a zone and allocate fixed pages
//! 	19.	Set the NoFurtherAlloc flag in a zone and allocate discardable pages
//! 	20.	Set the NoFurtherAlloc flag in a zone and allocate fixed pages to that zone using Epoc::ZoneAllocPhysicalRam()
//! 	21.	Set the NoFurtherAlloc flag in a zone and allocate fixed pages by calling TRamDefragRequest::ClaimRamZone()
//!		22.	Set up memory so that the least preferable RAM zone has movable pages + discardable pages > free pages in the most  
//!			preferable zone. Ensure that the discardable pages cannot be discarded and so must be moved. Now set the flag on  
//!			all zones barring the most preferable zone to KRamZoneFlagNoMovable, ensuring that most pref has no flags set. 
//!			Following this call a general defrag. 
//!		23.	Set up memory so that the least preferable RAM zone has movable pages + discardable pages > free pages in the most 
//!			preferable RAM zone. Ensure that the discardable pages cannot be discarded and so must be moved. 
//!			Now set the flag on all zones barring the most preferable zone to KRamZoneFlagNoDiscard, ensuring that 
//!			most preferable RAM zone has no flags set. Following this call a general defrag. 
//!		24. Set up memory so that the least preferable RAM zone has movable pages and discardable pages. Set all the zone 
//!			flags to KRamZoneFlagNoMovable. Following this call a general defrag. 
//!
//! @SYMTestExpectedResults
//! 	1.	Movable pages are allocated and no fixed pages allocated
//! 	2.	No fixed pages have been allocated
//! 	3.	Discardable pages are allocated and no fixed pages allocated
//!		4.	KErrNone, flag is ignored with zone specific allocation
//!		5.	KErrNone, flag is ignored when claiming a zone
//! 	6.	No movable pages have been allocated
//! 	7.	Fixed pages allocated, no movable allocated
//! 	8.	Discardable pages allocated, no movable pages allocated
//! 	9.	Movable pages allocated, no discardable pages allocated
//! 	10.	Fixed pages allocated, no discardable allocated
//! 	11.	No discardable pages allocated
//! 	12.	No movable pages allocated
//! 	13.	No fixed pages allocated
//! 	14.	Discardable pages allocated, no movable or fixed allocated
//!		15.	KErrNone, flag is ignored with zone specific allocation
//!		16.	KErrNone, flag is ignored when claiming a zone
//! 	17.	No moving, fixed or discardable pages allocated
//! 	18.	No moving, fixed or discardable pages allocated
//! 	19.	No moving, fixed or discardable pages allocated
//!		20.	KErrNoMemory, flag is obeyed with zone specific allocation
//!		21.	KErrNone, flag is ignored when claiming a zone
//!		22.	Movable pages moved to the most preferable zone, discardable pages moved to next most preferable zone
//!		23.	Discardable pages moved to most preferable zone, movable pages moved to next next most preferable zone
//!		24.	No pages are moved from the least preferable zone zone
//---------------------------------------------------------------------------------------------------------------------
TInt TestFlags()
	{
	TInt r = 0;
	TUint zoneDefragID = 0;

	test.Start(_L("Test1: No Fixed Flag, Alloc Movable"));	
	TestStart();	
	
	TInt index = GetBestZone(BEST_MOVABLE);

	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;
		if (SpaceAvailForPageTables(index, KFillAllMovable))
			{
			GetOriginalPageCount();
			r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_FIXED_FLAG);
			test_KErrNone(r);		

			AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable);
			GetAllPageInfo();
			if (gZoneUtilArray[index].iAllocMovable == gOriginalPageCountArray[index].iAllocMovable || 
				gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
				{
				test.Printf(_L("Fail: Either fixed pages have been allocated or movable pages have not been allocated to zone ID %x\n"), gZoneConfigArray[index].iZoneId);
				CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));	
				}
			}
		else
			{
			test.Printf(_L("Skipping...\n"));
			}
		}
	TestEnd();


	test.Next(_L("Test2: No Fixed Flag, Alloc Fixed"));	
	TestStart();

	if(gPagedRom)
		{
		r = DPTest::SetCacheSize(gOriginalMinCacheSize, gOriginalMinCacheSize);
		test_KErrNone(r);
		}

	// find a zone that has free pages in it to block it
	GetAllPageInfo();
	TUint i = 0;
	for (; i < gZoneCount; i++)
		{
		if (gZoneUtilArray[i].iFreePages > 0)
			{
			index = i;
			break;
			}
		}
	
	if (i == gZoneCount)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_FIXED_FLAG);
		test_KErrNone(r);

		GetOriginalPageCount();
		r = Ldd.AllocateFixed(FILL_ALL_FIXED);

		GetAllPageInfo();
		// Ensure that either zone does not contain extra fixed pages 
		if (gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
			{
			test.Printf(_L("Fail: Fixed pages have been allocated into the zone ID 0x%x r = %d\n"), zoneDefragID, r);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}	
	TestEnd();


	test.Next(_L("Test3: No Fixed Flag, Alloc Discardable"));	
	TestStart();
	if (gPagedRom)
		{
		index = GetBestZone(BEST_DISCARDABLE);
			
		if (index == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
			}
		else
			{
			zoneDefragID = gZoneConfigArray[index].iZoneId;
			GetOriginalPageCount();
			
			r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_FIXED_FLAG);
			test_KErrNone(r);

			UpdateRamInfo();
			TInt discardablePages;
			r = AllocDiscardable(discardablePages);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				TEST_FAIL;
				}
			
			GetAllPageInfo();
			if (gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
				{
				test.Printf(_L("Fail: Fixed pages have been allocated\n"));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));	
				}
			}		
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();
	
	test.Next(_L("Test4: No Fixed Flag, Alloc Fixed using ZoneAllocPhyicalRam"));	
	TestStart();
	
	index = GetBestZone(BEST_FIXED);
	
	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_FIXED_FLAG);
		test_KErrNone(r);		

		// Just need to try and allocate one page
		r = Ldd.ZoneAllocDiscontiguous(zoneDefragID, 1);

		GetAllPageInfo();
		if (r != KErrNone)
			{
			test.Printf(_L("Fail: Fixed pages have not been allocated into the zone\n"));
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}	
	TestEnd();

	test.Next(_L("Test5: No Fixed Flag, Alloc Fixed by attempting to claim zone"));	
	TestStart();
	
	GetAllPageInfo();
	index = 0;	
	while ((TUint)index < gZoneCount && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		index++;
		}
	
	if ((TUint)index == gZoneCount)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_FIXED_FLAG);
		test_KErrNone(r);		

		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, zoneDefragID);
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && r != KErrNone)
			{
			test.Printf(_L("Fail: Claim zone ID 0x%x was unsuccessful, r = %d\n"), zoneDefragID, r);
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}			
		}	
	TestEnd();
	


//------------------------------------------------------------
	test.Next(_L("Test6: No Movable Flag, Alloc Movable"));	
	TestStart();
	
	index = GetBestZone(BEST_MOVABLE);
	
	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_MOVE_FLAG);
		test_KErrNone(r);
		
		AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable, KChunkDefaultSize, EFalse);

		GetAllPageInfo();
		if (gZoneUtilArray[index].iAllocMovable >  gOriginalPageCountArray[index].iAllocMovable)
			{
			test.Printf(_L("Fail: Movable pages have been allocated in the zone\n"));
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}		
		}
	TestEnd();


	test.Next(_L("Test7: No Movable Flag, Alloc Fixed"));	
	TestStart();
	
	if(gPagedRom)
		{
		r = DPTest::SetCacheSize(gOriginalMinCacheSize, gOriginalMinCacheSize);
		test_KErrNone(r);
		}

	index = GetBestZone(BEST_FIXED);
		
	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{ 
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_MOVE_FLAG);
		test_KErrNone(r);

		GetOriginalPageCount();
		// Fill up all of RAM with fixed pages.
		r = Ldd.AllocateFixed(FILL_ALL_FIXED);
		test.Printf(_L("r = %d\n"), r);

		GetAllPageInfo();
		if (gZoneUtilArray[index].iAllocMovable > gOriginalPageCountArray[index].iAllocMovable || 
			gZoneUtilArray[index].iAllocFixed <= gOriginalPageCountArray[index].iAllocFixed)
			{
			test.Printf(_L("Fail: orig mov 0x%x fix 0x%x current mov 0x%x fix 0x%x\n"), gOriginalPageCountArray[index].iAllocMovable,
						gOriginalPageCountArray[index].iAllocFixed, gZoneUtilArray[index].iAllocMovable, gZoneUtilArray[index].iAllocFixed);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	TestEnd();


	test.Next(_L("Test8: No Movable Flag, Alloc Discardable"));	
	TestStart();
	if (gPagedRom)
		{	
		index = GetBestZone(BEST_DISCARDABLE);
		
		if (index == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
			}
		else
			{
			zoneDefragID = gZoneConfigArray[index].iZoneId;

			GetOriginalPageCount();
			r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_MOVE_FLAG);
			test_KErrNone(r)
			UpdateRamInfo();
			TInt discardablePages;
			r = AllocDiscardable(discardablePages);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				TEST_FAIL;
				}

			GetAllPageInfo();
			if (gZoneUtilArray[index].iAllocMovable > gOriginalPageCountArray[index].iAllocMovable)
				{
				test.Printf(_L("Fail: Movable pages have been allocated into the zone \n"));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));	
				}
			}
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();

//-----------------------------------------------------------------------------------------------

	test.Next(_L("Test9: No Discardable Flag, Alloc Movable"));	
	TestStart();
	
	index = GetBestZone(BEST_MOVABLE);

	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_DISCARD_FLAG);
		test_KErrNone(r);

		AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable);
		GetAllPageInfo();

		if (gZoneUtilArray[index].iAllocDiscardable > gOriginalPageCountArray[index].iAllocDiscardable || 
			(gZoneUtilArray[index].iAllocMovable <=  gOriginalPageCountArray[index].iAllocMovable &&
			gZoneUtilArray[index].iAllocFixed ==  gOriginalPageCountArray[index].iAllocFixed))
			{
			test.Printf(_L("Fail: Either discardable pages have been allocated or movable pages have not been allocated\n"));
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	TestEnd();


	test.Next(_L("Test10: No Discardable Flag, Alloc Fixed"));	
	TestStart();
	
	if(gPagedRom)
		{
		r = DPTest::SetCacheSize(gOriginalMinCacheSize, gOriginalMinCacheSize);
		test_KErrNone(r);
		}

	index = GetBestZone(BEST_FIXED);
			
	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{ 
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_DISCARD_FLAG);
		test_KErrNone(r);
		// Fill up all of RAM with fixed pages.
		r = Ldd.AllocateFixed(FILL_ALL_FIXED);

		GetAllPageInfo();
		if (gZoneUtilArray[index].iAllocDiscardable > gOriginalPageCountArray[index].iAllocDiscardable || 
			gZoneUtilArray[index].iAllocFixed <= gOriginalPageCountArray[index].iAllocFixed)
			{
			test.Printf(_L("Fail: Discardable pages have been allocated or fixed pages have not been allocated\n"));
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	TestEnd();


	test.Next(_L("Test11: No Discardable Flag, Alloc Discardable"));	
	TestStart();
	if (gPagedRom)
		{
		index = GetBestZone(BEST_DISCARDABLE);
			
		if (index == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
			}
		else
			{
			zoneDefragID = gZoneConfigArray[index].iZoneId;
			GetOriginalPageCount();
			r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_DISCARD_FLAG);
			test_KErrNone(r);
			UpdateRamInfo();
			
			TInt discardablePages;
			r = AllocDiscardable(discardablePages);
			if (r != KErrNoMemory)
				{// Allocation should fail as no dis flag is set
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				TEST_FAIL;
				}
			GetAllPageInfo();

			if (gZoneUtilArray[index].iAllocDiscardable > gOriginalPageCountArray[index].iAllocDiscardable)
				{
				test.Printf(_L("Fail: Discardable pages have been allocated into the zone\n"));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));	
				}
			}
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();

//-----------------------------------------------------------------------------------------------

	test.Next(_L("Test12: Only Discardable Flag, Alloc Movable"));	
	TestStart();
	
	index = GetBestZone(BEST_MOVABLE);

	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, ONLY_DISCARD_FLAG);
		test_KErrNone(r);
		AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable, KChunkDefaultSize, EFalse);
		GetAllPageInfo();

		if (gZoneUtilArray[index].iAllocMovable > gOriginalPageCountArray[index].iAllocMovable)
			{
			test.Printf(_L("Fail: Movable pages have been allocated\n"));
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	TestEnd();


	test.Next(_L("Test13: Only Discardable Flag, Alloc Fixed"));	
	TestStart();
	
	if(gPagedRom)
		{
		r = DPTest::SetCacheSize(gOriginalMinCacheSize, gOriginalMinCacheSize);
		test_KErrNone(r);
		}

	index = GetBestZone(BEST_FIXED);

	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, ONLY_DISCARD_FLAG);
		test_KErrNone(r);

		// Fill up all of RAM with fixed pages.
		r = Ldd.AllocateFixed(FILL_ALL_FIXED);

		GetAllPageInfo();
		if (gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
			{
			test.Printf(_L("Fail: Fixed pages have been allocated\n"));
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}	
		}
	TestEnd();


	test.Next(_L("Test14: Only Discardable Flag, Alloc Discardable"));	
	TestStart();
	if (gPagedRom)
		{
		index = GetBestZone(BEST_DISCARDABLE);

		if (index == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
			}
		else
			{
			zoneDefragID = gZoneConfigArray[index].iZoneId;

			GetOriginalPageCount();
			r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, ONLY_DISCARD_FLAG);
			test_KErrNone(r);
			UpdateRamInfo();
			
			TInt discardablePages;
			r = AllocDiscardable(discardablePages);
			if (r != KErrNone)
				{
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				TEST_FAIL;
				}
			GetAllPageInfo();

			if (gZoneUtilArray[index].iAllocMovable > gOriginalPageCountArray[index].iAllocMovable ||
				gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
				{
				test.Printf(_L("Fail: Pages other than discardable have been allocated\n"));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));	
				}
			}
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();

	test.Next(_L("Test15: Only Discardable Flag, Alloc Fixed using ZoneAllocPhyicalRam"));	
	TestStart();
	
	index = GetBestZone(BEST_FIXED);
	
	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, ONLY_DISCARD_FLAG);
		test_KErrNone(r);		

		// Just need to try and allocate one page
		r = Ldd.ZoneAllocDiscontiguous(zoneDefragID, 1);

		GetAllPageInfo();
		if (r != KErrNone)
			{
			test.Printf(_L("Fail: Fixed pages have not been allocated into zone ID 0x%x\n"), zoneDefragID);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}	
		}	
	TestEnd();

	test.Next(_L("Test16: Only Discardable Flag, Alloc Fixed by attempting to claim zone"));	
	TestStart();
	
	GetAllPageInfo();
	index = 0;	
	while ((TUint)index < gZoneCount && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		index++;
		}
	
	if ((TUint)index == gZoneCount)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, ONLY_DISCARD_FLAG);
		test_KErrNone(r);		

		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, zoneDefragID);
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && r != KErrNone)
			{
			test.Printf(_L("Fail: Claim zone ID 0x%x was unsuccessful, r = %d\n"), zoneDefragID, r);
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}		
		}	
	TestEnd();
//-----------------------------------------------------------------------------------------------

	test.Next(_L("Test17: No further alloc Flag, Alloc Movable"));	
	TestStart();
	
	index = GetBestZone(BEST_MOVABLE);

	
	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_ALLOC_FLAG);
		test_KErrNone(r);
		AllocMovable(gChunkArray1, gChunkArraySize1, KFillAllMovable, KChunkDefaultSize, EFalse);
		GetAllPageInfo();

		if (gZoneUtilArray[index].iAllocDiscardable > gOriginalPageCountArray[index].iAllocDiscardable || 
			gZoneUtilArray[index].iAllocMovable >  gOriginalPageCountArray[index].iAllocMovable || 
			gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
			{
			test.Printf(_L("Fail: Pages been allocated\n"));
			CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	TestEnd();


	test.Next(_L("Test18: No further alloc Flag, Alloc Fixed"));	
	TestStart();
	
	if(gPagedRom)
		{
		r = DPTest::SetCacheSize(gOriginalMinCacheSize, gOriginalMinCacheSize);
		test_KErrNone(r);
		}

	// Find a zone with free pages and set the flag on this zone 
	// as it will ensure that you cannot fill all of free RAM with fixed pages
	index = KErrNotFound;
	GetAllPageInfo();
	for (TUint i = 0; i < gZoneCount; i++)
		{
		if (gZoneUtilArray[i].iFreePages != 0)
			{
			index = i;
			break;
			}
		}
	if (index == KErrNotFound)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{ 
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_ALLOC_FLAG);
		test_KErrNone(r);

		GetOriginalPageCount();
		r = Ldd.AllocateFixed(FILL_ALL_FIXED);
		
		// Ensure memory wasn't filled as it should have hit the blocked zone.
		GetAllPageInfo();
		if (r != KErrNoMemory || gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
			{
			test.Printf(_L("Fail: orig fix 0x%x current fix 0x%x\n"), gOriginalPageCountArray[index].iAllocFixed, gZoneUtilArray[index].iAllocFixed);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}
	TestEnd();


	test.Next(_L("Test19: No further alloc Flag, Alloc Discardable"));	
	TestStart();
	if (gPagedRom)
		{
		index = GetBestZone(BEST_DISCARDABLE);
		
		if (index == KErrNotFound)
			{
			test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
			}
		else
			{
			zoneDefragID = gZoneConfigArray[index].iZoneId;

			GetOriginalPageCount();
			r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_ALLOC_FLAG);
			test_KErrNone(r);
			UpdateRamInfo();
			TInt discardablePages;
			r = AllocDiscardable(discardablePages);
			if (r != KErrNoMemory)
				{// Allocation should fail as no alloc flag is set
				test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
				TEST_FAIL;
				}
			GetAllPageInfo();

			if (gZoneUtilArray[index].iAllocDiscardable > gOriginalPageCountArray[index].iAllocDiscardable || 
				gZoneUtilArray[index].iAllocMovable >  gOriginalPageCountArray[index].iAllocMovable || 
				gZoneUtilArray[index].iAllocFixed > gOriginalPageCountArray[index].iAllocFixed)
				{
				test.Printf(_L("Fail: Pages have been allocated into the zone\n"));
				CLEANUP(ResetDPCache());
				TEST_FAIL;
				}
			else
				{
				test.Printf(_L("Passed...\n"));	
				}
			}
		}
	else
		{
		test.Printf(_L("Not a paged ROM - Skipping test step\n"));
		}
	TestEnd();

	
	test.Next(_L("Test20: No Further Alloc Flag, Alloc Fixed using ZoneAllocPhyicalRam"));	
	TestStart();
	
	GetAllPageInfo();
	index = 0;
	while ((TUint)index < gZoneCount && (gZoneUtilArray[index].iFreePages == 0 || 
			gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		index++;
		}
	
	if ((TUint)index == gZoneCount)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;

		GetOriginalPageCount();
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_ALLOC_FLAG);
		test_KErrNone(r);		

		// Just need to try and allocate one page
		r = Ldd.ZoneAllocDiscontiguous(zoneDefragID, 1);

		GetAllPageInfo();
		if (r != KErrNoMemory)
			{
			test.Printf(_L("Fail: Fixed pages have been allocated into zone ID 0x%x\n"), zoneDefragID);
			CLEANUP(Ldd.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}	
	TestEnd();

	test.Next(_L("Test21: No Further Alloc Flag, Alloc Fixed by attempting to claim zone"));	
	TestStart();
	
	GetAllPageInfo();
	index = 0;	
	while ((TUint)index < gZoneCount && (gZoneUtilArray[index].iAllocFixed != 0 || gZoneUtilArray[index].iAllocUnknown != 0))
		{
		index++;
		}
	
	if ((TUint)index == gZoneCount)
		{
		test.Printf(_L("Cannot find zone to perform test - Skipping test step...\n"));
		}
	else
		{
		zoneDefragID = gZoneConfigArray[index].iZoneId;
		r = Ldd.SetZoneFlag(zoneDefragID, gZoneConfigArray[index].iFlags, NO_ALLOC_FLAG);
		test_KErrNone(r);		

		r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, zoneDefragID);
		GetAllPageInfo();
		TUint freeInOthers = gTotalPageCount.iFreePages - gZoneUtilArray[index].iFreePages;
		if (freeInOthers && r != KErrNone)
			{
			test.Printf(_L("Fail: Claim zone ID 0x%x was unsuccessful, r = %d\n"), zoneDefragID, r);
			CLEANUP(ResetRamZoneFlags());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));	
			}
		}	
	TestEnd();

	TUint mostPrefArrayIndex = 0;
	TUint mostPrefIndex = 0;
	TUint leastPrefIndex = 0;
	const TUint KFreeMostPref = 10;
	TUint flag = 0;
	TUint prefIndex;
	TUint totalFree;


	for (TUint testStep = 0; testStep < 2; testStep++)
		{
		switch(testStep)
			{
			case 0:
				test.Next(_L("Test22: Ensure that the General Defrag looks at the flags 1"));
				break;

			case 1:
				test.Next(_L("Test23: Ensure that the General Defrag looks at the flags 2"));
				break;
			}
			
		TestStart();
		gChunkArray1 = new RChunk;	
		gChunkArraySize1 = 1;
		TUint minCacheSize = 0;
		TUint maxCacheSize = 0;
		TUint currentCacheSize = 0;
		TUint freeNeededInMostPref = 0;
		if (!gPagedRom)
			{
			test.Printf(_L("Not a paged ROM - Skipping...\n"));
			goto skipTest22;
			}

		// Find the most pref zone with free pages
		r = FindMostPrefWithFree(mostPrefIndex, &mostPrefArrayIndex);
		if (r != KErrNone)
			{
			test.Printf(_L("Cannot find zone with free pages - Skipping...\n"));
			goto skipTest22;
			}

		// Ensure that the least pref zone is empty
		leastPrefIndex = gPrefArray[gZoneCount - 1];
		if (gZoneUtilArray[leastPrefIndex].iFreePages != gZoneUtilArray[leastPrefIndex].iPhysPages)
			{
			test.Printf(_L("Least pref zone is not empty - Skipping...\n"));
			goto skipTest22;
			}

		// Allocate 1 movable page to the least preferable zone
		r = ZoneAllocMovable(gChunkArray1, gChunkArraySize1, leastPrefIndex, 1);
		if (r != KErrNone)
			{			
			test.Printf(_L("Failed to allocate movable r = %d - Skipping...\n"), r);
			goto skipTest22;
			}
		
		// Allocate 1 discardable page to the least preferable zone
		if (gZoneUtilArray[leastPrefIndex].iFreePages != 0)
			{
			TInt disPages;
			r = ZoneAllocDiscard(leastPrefIndex, 1, disPages);
			if (r != KErrNone)
				{			
				test.Printf(_L("Failed to allocate discardable pages r = %d - Skipping...\n"), r);
				goto skipTest22;
				}

			// up the minimum cache size so that the pages have to be moved - not discarded
			DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
			r = DPTest::SetCacheSize(currentCacheSize, currentCacheSize);
			if (r != KErrNone)
				{			
				test.Printf(_L("Failed to set cache size r = %d - Skipping...\n"), r);
				goto skipTest22;
				}
			}
		else
			{
			test.Printf(_L("Least pref zone has no free pages - Skipping...\n"));
			goto skipTest22;
			}

		// Check that the least pref zone has movable and discardable pages in it
		GetAllPageInfo();
		if (gZoneUtilArray[leastPrefIndex].iAllocMovable == 0 ||
			gZoneUtilArray[leastPrefIndex].iAllocDiscardable == 0)
			{
			test.Printf(_L("No movable or discardable in least pref zone\n"));
			PrintPageInfo();
			goto skipTest22;
			}

		ResetRamZoneFlags();
		// if most pref zone has too many free pages fill up with fixed
		if (gZoneUtilArray[mostPrefIndex].iFreePages > KFreeMostPref)
			{
			TUint allocPages = gZoneUtilArray[mostPrefIndex].iFreePages - KFreeMostPref;
			r = Ldd.ZoneAllocToMany(mostPrefIndex, allocPages);
			if (r != KErrNone)
				{	
				test.Printf(_L("Failed allocate 0x%x fixed to index %d r = %d - Skipping...\n"), 
								allocPages, mostPrefIndex,r);
				PrintPageInfo();
				goto skipTest22;
				}
			}
		
		
		// if the no. of discardable pages is less than free in most pref increase the min cache size
		GetAllPageInfo();
		if (gZoneUtilArray[leastPrefIndex].iAllocDiscardable < gZoneUtilArray[mostPrefIndex].iFreePages)
			{
			TUint discDiff = gZoneUtilArray[mostPrefIndex].iFreePages - gZoneUtilArray[leastPrefIndex].iAllocDiscardable;
			test.Printf(_L("discDiff = 0x%x\n"), discDiff);
			TInt disPages;
			if (ZoneAllocDiscard(leastPrefIndex, discDiff, disPages) != KErrNone)
				{			
				test.Printf(_L("Failed allocate discardable to zone index %d- Skipping...\n"), leastPrefIndex);
				goto skipTest22;
				}
			// up the minimum cache size by the difference as we don't want these pages to be discarded
			DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
			r = DPTest::SetCacheSize(currentCacheSize, currentCacheSize);
			test.Printf(_L("r = %d\n"), r);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to set cache size r = %d - Skipping...\n"), r);
				goto skipTest22;
				}
			}
		ResetRamZoneFlags();

		// if the rest of the zones are either full or empty put a fixed page into the next most pref empty zone
		GetPrefOrder();
		prefIndex = 0;
		totalFree = 0;
		for (; prefIndex < gZoneCount && totalFree < gZoneUtilArray[leastPrefIndex].iAllocDiscardable; prefIndex++)
			{// Look for zone that has enough free pages to fit all the discardable
			TUint zoneIndex = gPrefArray[prefIndex];
			if (zoneIndex != mostPrefIndex && zoneIndex != leastPrefIndex)
				{
				if (gZoneUtilArray[zoneIndex].iFreePages != gZoneUtilArray[zoneIndex].iPhysPages)
					{
					totalFree += gZoneUtilArray[zoneIndex].iFreePages;
					}
				else
					{
					r = Ldd.ZoneAllocToMany(zoneIndex, 1);
					if (r != KErrNone)
						{
						test.Printf(_L("Couldn't alloc fixed to zone index %d - r = %d\n"), zoneIndex, r);
						goto skipTest22;
						}
					GetAllPageInfo();
					totalFree += gZoneUtilArray[zoneIndex].iFreePages;
					}
				}
			}

		if (prefIndex >= gZoneCount)
			{
			test.Printf(_L("Couldn't find zone\n"));
			goto skipTest22;
			}


		
		// If the most preferable zone does not have enough free pages, skip
		freeNeededInMostPref = (testStep == 0) ? gZoneUtilArray[leastPrefIndex].iAllocMovable : gZoneUtilArray[leastPrefIndex].iAllocDiscardable;
		if (gZoneUtilArray[mostPrefIndex].iFreePages < freeNeededInMostPref)
			{
			test.Printf(_L("Free needed in mostPref(%d) = %d, Free available in mostPref = %d - skipping...\n"), 
							mostPrefIndex,freeNeededInMostPref,gZoneUtilArray[mostPrefIndex].iFreePages);
			goto skipTest22;
			}

		GetAllPageInfo();
		// Set up the RAM zone flags for the test
		flag = (testStep == 0)? NO_MOVE_FLAG: NO_DISCARD_FLAG;
		// Set all zones except most pref to KRamZoneFlagNoMovable	or KRamZoneFlagNoDiscard
		for (TUint index = 0; index < gZoneCount; index++)
			{
			TUint zoneID = gZoneConfigArray[index].iZoneId;
			if (index != mostPrefIndex)
				{
				r = Ldd.SetZoneFlag(zoneID, gZoneConfigArray[index].iFlags, flag);
				if (r != KErrNone)
					{			
					test.Printf(_L("Failed to set flag r = %d - Skipping...\n"), r);
					goto skipTest22;
					}
				}
			}
		
		GetOriginalPageCount();
		r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		
		GetAllPageInfo();
		switch(testStep)
			{
			case 0:
				if (r != KErrNone ||
					gOriginalPageCountArray[mostPrefIndex].iAllocMovable >= gZoneUtilArray[mostPrefIndex].iAllocMovable ||
					gZoneUtilArray[leastPrefIndex].iFreePages != gZoneUtilArray[leastPrefIndex].iPhysPages)
					{
					test.Printf(_L("FAIL:r=%d MostPref(%d): origMov 0x%x curMov 0x%x LeastPref(%d): origMov 0x%x curMov 0x%x origDis 0x%x curDis 0x%x\n"), 
									r, mostPrefIndex, gOriginalPageCountArray[mostPrefIndex].iAllocMovable, gZoneUtilArray[mostPrefIndex].iAllocMovable, 
									leastPrefIndex, gOriginalPageCountArray[leastPrefIndex].iAllocMovable, gZoneUtilArray[leastPrefIndex].iAllocMovable,
									gOriginalPageCountArray[leastPrefIndex].iAllocDiscardable, gZoneUtilArray[leastPrefIndex].iAllocDiscardable);
					CLEANUP(Ldd.FreeAllFixedPages());
					TEST_FAIL;
					}
				else
					{
					test.Printf(_L("Passed...\n"));
					}
				break;

			case 1:
				if (r != KErrNone ||
					gOriginalPageCountArray[mostPrefIndex].iAllocDiscardable >= gZoneUtilArray[mostPrefIndex].iAllocDiscardable ||
					gZoneUtilArray[leastPrefIndex].iFreePages != gZoneUtilArray[leastPrefIndex].iPhysPages)
					{
					test.Printf(_L("FAIL:r=%d MostPref(%d): origMov 0x%x curMov 0x%x LeastPref(%d): origMov 0x%x curMov 0x%x origDis 0x%x curDis 0x%x\n"), 
									r, mostPrefIndex, gOriginalPageCountArray[mostPrefIndex].iAllocMovable, gZoneUtilArray[mostPrefIndex].iAllocMovable, 
									leastPrefIndex, gOriginalPageCountArray[leastPrefIndex].iAllocMovable, gZoneUtilArray[leastPrefIndex].iAllocMovable,
									gOriginalPageCountArray[leastPrefIndex].iAllocDiscardable, gZoneUtilArray[leastPrefIndex].iAllocDiscardable);
					CLEANUP(Ldd.FreeAllFixedPages());
					TEST_FAIL;
					}
				else
					{
					test.Printf(_L("Passed...\n"));
					}
				break;
			}

	skipTest22:
		TestEnd();
		}

	test.Next(_L("Test24: Ensure that the General Defrag doesnt move or discard pages if KRamZoneFlagNoMovable set on all zones "));	
	TestStart();
	gChunkArray1 = new RChunk;
	gChunkArraySize1 = 1;
	// Find the most pref zone with free pages
	GetPrefOrder();
	TInt disPages = 0;	
	if (FindMostPrefWithFree(mostPrefIndex, &mostPrefArrayIndex) != KErrNone)
		{
		test.Printf(_L("Cannot find zone with free pages - Skipping...\n"));
		goto skipTest24;
		}

	// Ensure that the least pref zone has free pages in it
	leastPrefIndex = gPrefArray[gZoneCount-1];
	if (gZoneUtilArray[leastPrefIndex].iFreePages == 0)
		{
		test.Printf(_L("Least pref zone has no free pages - Skipping...\n"));
		goto skipTest24;
		}

	// Allocate 1 movable page to the least preferable zone
	r = ZoneAllocMovable(gChunkArray1, gChunkArraySize1, leastPrefIndex, 1);
	if (r != KErrNone)
		{			
		test.Printf(_L("Failed to allocate movable page r = %d - Skipping...\n"), r);
		goto skipTest24;
		}
	
	if (gPagedRom)
		{
		
		TUint minCacheSize = 0;
		TUint maxCacheSize = 0;
		TUint currentCacheSize = 0;
		
		// Allocate 1 discardable page to the least preferable zone
		if (gZoneUtilArray[leastPrefIndex].iFreePages != 0)
			{
			r = ZoneAllocDiscard(leastPrefIndex, 1, disPages);
			if (r != KErrNone)
				{
				test.Printf(_L("Discardable pages not allocated r= %d\n"), r);
				}
			
			DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
			r = DPTest::SetCacheSize(currentCacheSize, currentCacheSize);
			if (r != KErrNone)
				{			
				test.Printf(_L("Failed to set cache size r = %d - Skipping...\n"), r);
				goto skipTest24;
				}
			}	
		}

	
	// Check that the least pref zone has movable and discardable (if allocated) pages in it
	GetAllPageInfo();
	if (gZoneUtilArray[leastPrefIndex].iAllocMovable == 0 ||
		gZoneUtilArray[leastPrefIndex].iAllocDiscardable < (TUint)disPages ||
		gZoneUtilArray[leastPrefIndex].iAllocMovable > gTotalPageCount.iFreePages - gZoneUtilArray[leastPrefIndex].iFreePages)
		{
		test.Printf(_L("No movable in least pref zone or no space for moveable pages to be moved to\n"));
		PrintPageInfo();
		goto skipTest24;
		}

	ResetRamZoneFlags();
	GetAllPageInfo();
	// Now set all zones to KRamZoneFlagNoMovable
	for (TUint index = 0; index < gZoneCount; index++)
		{
		TUint zoneID = gZoneConfigArray[index].iZoneId;
		r = Ldd.SetZoneFlag(zoneID, gZoneConfigArray[index].iFlags, NO_MOVE_FLAG);
		if (r != KErrNone)
			{			
			test.Printf(_L("Failed to set cache size r = %d - Skipping...\n"), r);
			goto skipTest24;
			}
		}

	GetOriginalPageCount();
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	GetAllPageInfo();
	if (r != KErrNone ||
		gOriginalPageCountArray[leastPrefIndex].iAllocMovable != gZoneUtilArray[leastPrefIndex].iAllocMovable ||
		gZoneUtilArray[leastPrefIndex].iAllocDiscardable < gOriginalPageCountArray[leastPrefIndex].iAllocDiscardable)
		{
		test.Printf(_L("FAIL:r=%d LeastPref(%d): origMov 0x%x curMov 0x%x origDis 0x%x curDis 0x%x\n"), 
						r,leastPrefIndex, gOriginalPageCountArray[leastPrefIndex].iAllocMovable, gZoneUtilArray[leastPrefIndex].iAllocMovable,
						gOriginalPageCountArray[leastPrefIndex].iAllocDiscardable, gZoneUtilArray[leastPrefIndex].iAllocDiscardable);
		CLEANUP(ResetRamZoneFlags());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));
		}
skipTest24:
	TestEnd();

	test.End();
	return KErrNone;
	}


//
// Template functions encapsulating ControlIo magic
//
template <class C>
TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
	{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
	}

//
// FormatMMC
//
// Formats the MMC card
//
void FormatMMC()
	{	
	test.Printf(_L("Formatting MMC...\n"));

	RFs theFs;
	TBuf<4> driveBuf = _L("D:\\");
	RFormat format;
	TInt count;
	TChar driveLet;

	TInt r = theFs.Connect();
	test_KErrNone(r);
	
	r = theFs.DriveToChar(gDrive, driveLet);
	test_KErrNone(r);

	driveBuf[0] = driveLet;
	test.Printf(_L("Formatting Drive: %C\n"),(TInt)driveLet);
	
	r = format.Open(theFs,driveBuf,EFullFormat,count);
	test_KErrNone(r);
	
	while(count)
		{
		TInt r = format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	theFs.Close();
	}


//
// FileNameGen
//
// Generates file names to create the files
//
void FileNameGen(TDes16& aBuffer, TInt aLong, TInt aPos) 
	{
	TInt padding;
	TInt i = 0;
	TBuf16<10> tempbuf;

	_LIT(KNumber,"%d");
	tempbuf.Format(KNumber,aPos);
	
	padding = aLong-tempbuf.Size()/2;
	aBuffer = _L("");
	
	while(i < padding)
		{
		aBuffer.Append('F');
		i++;
		}
	aBuffer.Append(tempbuf);

	_LIT(KExtension1, ".TXT");
	aBuffer.Append(KExtension1);
	}


//
// CreateFiles
//
// Creates the files to fill part of the read cache
//
void CreateFiles(TInt aFiles, TInt aFileSize)
	{
	TInt i = 0, r = 0;
	RFile file;
	TBuf16<50> directory;
	
	TBuf16<50> path;
	TBuf16<50> buffer(50); 	
	
	directory = gSessionPath;
	
	test.Printf(_L("Creating %d files for filling the cache (size %d)\n"), aFiles, aFileSize);

	// create a big buffer to speed things up
	HBufC8* bigBuf = NULL;
	const TInt KBigBufferSize = 32 * 1024;
	TRAPD(res,bigBuf = HBufC8::NewL(KBigBufferSize));
	test(res == KErrNone && bigBuf != NULL);
		
	TPtr8 bigBufWritePtr(NULL, 0);	
	bigBufWritePtr.Set(bigBuf->Des());

	// Fill the buffer
	TChar aC = 'A';
	for(i = 0; i < KBigBufferSize; i++)
		{
		bigBufWritePtr.Append((i%32) + aC);
		}
	

	i = 0;		
	while(i < aFiles) 
		{
		if (i % 10 == 0)
			test.Printf(_L("Creating file %d of %d...\r"), i, aFiles);
		FileNameGen(buffer, 8, i+3) ;
		path = directory;
		path.Append(buffer);

		// delete file first to ensure it's contents are not in the cache (file may be on the closed file queue)
		r = gTheFs.Delete(path);
		test(r == KErrNone || r == KErrNotFound);

		r = file.Create(gTheFs,path,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
		if(r == KErrAlreadyExists) 
			r = file.Open(gTheFs,path,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
		TInt j = 0;
		i++;
		while(j < aFileSize)
			{
			bigBufWritePtr.SetLength(Min(KBigBufferSize, aFileSize - j));
			r = file.Write(bigBufWritePtr);
			// Running out of disk space is expected for the last file.
			// Premature "disk full" conditions need to abort.
			if (r == KErrDiskFull)
				{
				test(i == aFiles);
				break;
				}
			test_KErrNone(r);
			j += bigBufWritePtr.Length();
			}					

		file.Close();
		}
	test.Printf(_L("\nFiles created\n"));
	delete bigBuf;
	}


//
// FillCache
//
// Allocate discardable pages using file system caching
//
TInt FillCache(TInt aFiles, TInt aFileSize)
	{
	// Fail if files already open
	test(!gFileCacheRun);


	TInt i = 0, r = 0;
	TBuf16<50> directory;
	
	TBuf16<50> path;
	TBuf16<50> buffer(50); 	
	HBufC8* buf = NULL;
	TPtr8 bufPtr(NULL, 0);	
	
	TRAPD(res,buf = HBufC8::NewL(2));
	test(res == KErrNone && buf != NULL);
	bufPtr.Set(buf->Des());
	
	TESTDEBUG(test.Printf(_L("Filling the cache\n")));

	directory = gSessionPath;
	
	i = 0;		

	GetAllPageInfo();
	TESTDEBUG(test.Printf(_L("total disc pages = %d\n"), gTotalPageCount.iDiscardablePages)); 
	

	while(i < aFiles) 
		{
		FileNameGen(buffer, 8, i+3) ;
		path = directory;
		path.Append(buffer);
		r = gFile[i].Open(gTheFs,path,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
		test_KErrNone(r);
		
		TInt j = 0;
		while(j < aFileSize)
			{
			r = gFile[i].Read(j,bufPtr);
			test_KErrNone(r);
			j += 4 * 1024;
			}					
		i++;
		}
	gFileCacheRun = ETrue;
	GetAllPageInfo();
	TESTDEBUG(test.Printf(_L("after - total disc pages = %d\n"), gTotalPageCount.iDiscardablePages)); 
	delete buf;
	TESTDEBUG(test.Printf(_L("Cache filled\n")));
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// get number of items on Page Cache
	TFileCacheStats startPageCacheStats;

	r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
	test.Printf(_L("control stats r= %d\n"), r);
	test(r==KErrNone || r == KErrNotSupported);
	
	TESTDEBUG(test.Printf(_L("Allocated segment count=%d\n"),startPageCacheStats.iAllocatedSegmentCount));
#endif
	// if we do not have any discardable pages then something went
	// wrong with file caching
	if (gTotalPageCount.iDiscardablePages == 0)
		return KErrNotSupported;
		
	return KErrNone;
	}
	


//
// SetUpMMC
//
// Sets up the MMC to be used by the test by creating files on the MMC
//
void SetUpMMC()
	{
	FormatMMC();

	TInt r = 0;

	TChar driveToTest;	
	
	TVolumeInfo volInfo;
	
	r = gTheFs.DriveToChar(gDrive, driveToTest);
	test_KErrNone(r);
					
	r = gTheFs.CharToDrive(driveToTest,gDrive);
	test_KErrNone(r);

	gSessionPath = _L("?:\\F32-TST\\");
	gSessionPath[0] = (TUint16) driveToTest;
	test.Printf(_L("Drive Letter=%C\n"),(TInt)driveToTest);

	TDriveInfo info;
	r = gTheFs.Drive(info,gDrive);
	test_KErrNone(r);
	r = gTheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	r = gTheFs.MkDirAll(gSessionPath);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		test_KErrNone(r);
		}
	
	r = gTheFs.Volume(volInfo, gDrive);
	test_KErrNone(r);
	TInt64 gMediaSize = volInfo.iSize;

	// This calculation is approximate because the client cannot know
	// internal allocation mechanisms of the filesystem, i.e. how much
	// metadata is associated with a file of name X / size Y, whether
	// space used by such metadata is reflected in TDriveInfo::iSize and
	// what block/clustersize the filesystem will round filesizes to.
	// The last file that fills up the drive may therefore be partial
	// (smaller than this calculation predicts).
	TInt maxPossibleFiles = gFilesNeededToFillCache;
	test.Printf(_L("Original files needed = %d\n"), maxPossibleFiles);
	if(gMediaSize < (KDefaultCacheSize * maxPossibleFiles))
		{
		maxPossibleFiles = (gMediaSize - 10) / KDefaultCacheSize;
		test.Printf(_L("Disk size is smaller - files needed = %d\n"), maxPossibleFiles);
		}
	gFilesNeededToFillCache = maxPossibleFiles;
	CreateFiles(gFilesNeededToFillCache, KDefaultCacheSize);
	}


//
// TestFileCaching
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0599
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying that when File System Caching allocates dicardable pages, 
//! 							Defrag and allocation of fixed pages happens correctly. 
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fill the file system cache to allocate discardable pages
//! 		following this call EmptyRamZone() in a zone with discardable pages. 
//! 	2.	Fill the file system cache to allocate discardable pages
//! 		folling this allocate discontiguous fixed pages to a zone
//! 	3.	Fill the file system cache to allocate discardable pages
//! 	.	following this allocate discontuguous fixed pages
//! 	4.	Fill the file system cache to allocate discardable pages
//! 		following this allocate less than 16 contiguous fixed pages
//! 	5.	Fill the file system cache to allocate discardable pages
//! 		following this allocate more than 16 contiguous fixed pages
//!
//! @SYMTestExpectedResults
//! 	1.	Discardable pages are removed
//! 	2.	KErrNone
//! 	3.	KErrNone
//! 	4.	KErrNone and numDiscardablePages != 0
//! 	5.	KErrNone and numDiscardablePages = 0
//---------------------------------------------------------------------------------------------------------------------
TInt TestFileCaching()
	{
	const TUint KDisPagesReq = 1;
	TInt r = KErrNone;
	TInt allocSize = 0;
	TUint zoneID = 0;
	TUint index = 0;
	TUint minDiscardPages = 0;
	if (gDrive == KNoRemovableDrive)
		{
		test.Start(_L("Cannot find drive to write files to - Skipping FS Caching Tests\n"));
		test.End();
		return 0;
		}
	
	r = gTheFs.Connect();
	test_KErrNone(r);

	SetUpMMC();
	
	RRamDefragFuncTestLdd Ldd2;
	r = Ldd2.Open();
	test_KErrNone(r);
	
	test.Start(_L("Test1: Test EmptyRamZone() clears file server cache pages"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	r = FillCache(gFilesNeededToFillCache, KDefaultCacheSize);
	if (r != KErrNone)
		{
		test.Printf(_L("File system caching failed - Skipping all file caching tests...\n"));
		goto skipFileCacheTests;
		}
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	
	GetAllPageInfo();
	while (index < gZoneCount && gZoneUtilArray[index].iAllocDiscardable < KDisPagesReq)
		{
		++ index;
		}
	
	if (index == gZoneCount)  
		{
		test.Printf(_L("Cannot find zone to perform test on - Skipping test step...\n"));
		}
	else
		{
		zoneID = gZoneConfigArray[index].iZoneId;

		r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, zoneID);
		
		GetAllPageInfo();
		if (gZoneUtilArray[index].iAllocDiscardable != 0)
			{
			test.Printf(_L("Fail: Zone ID 0x%x has 0x%x discardable pages\n"), zoneID, gZoneUtilArray[index].iAllocDiscardable);
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));
			}
		}
	TestEnd();

	test.Next(_L("Test2: Filling the FS Cache and allocating fixed pages to a zone"));	
	TestStart();
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	r = FillCache(gFilesNeededToFillCache, KDefaultCacheSize);
	if (r != KErrNone)
		{
		test.Printf(_L("File system caching failed - Skipping all file caching tests...\n"));
		goto skipFileCacheTests;
		}
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();	
	index = 0;
	while (index < gZoneCount && gZoneUtilArray[index].iAllocDiscardable < KDisPagesReq)
		{
		++ index;
		} 

	if (index == gZoneCount)  
		{
		test.Printf(_L("Cannot find zone to perform test on - Skipping test step...\n"));
		}
	else
		{
		zoneID = gZoneConfigArray[index].iZoneId;

		// Just need to attempt to allocate one more page than there is free in the zone
		allocSize = gZoneUtilArray[index].iFreePages + 1;
		
		test.Printf(_L("Allocating 0x%x fixed pages to zone ID 0x%x.....\n"), allocSize, zoneID);
		r = Ldd.ZoneAllocDiscontiguous(zoneID, allocSize);
		test.Printf(_L("r = %d\n"), r);

		GetAllPageInfo();
		if (r != KErrNone)
			{
			test.Printf(_L("Fail: Fixed pages have not been allocated, r = %d, expected = %d\n"), r, KErrNone);
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));
			}
		TESTDEBUG(test.Printf(_L("Freeing all Fixed Pages.....\n")));
		Ldd.FreeAllFixedPages();
		}	
	TestEnd();


	test.Next(_L("Test3: Filling the FS Cache and allocating fixed pages"));	
	TestStart();
	r = FillCache(gFilesNeededToFillCache, KDefaultCacheSize);
	if (r != KErrNone)
		{
		test.Printf(_L("File system caching failed - Skipping all file caching tests...\n"));
		goto skipFileCacheTests;
		}

	GetAllPageInfo();	
	index = 0;
	while (index < gZoneCount && gZoneUtilArray[index].iAllocDiscardable < KDisPagesReq)
		{
		++ index;
		}
		
	if (index == gZoneCount)  
		{
		test.Printf(_L("Cannot find zone to perform test on - Skipping test step...\n"));
		}
	else
		{ 
		zoneID = gZoneConfigArray[index].iZoneId;

		allocSize = 14; 

		TESTDEBUG(test.Printf(_L("Filling the remaining free pages with fixed pages.....\n")));
		Ldd.AllocateFixed(gTotalPageCount.iFreePages);
		
		test.Printf(_L("Allocating 0x%x fixed pages to zone ID 0x%x.....\n"), allocSize, zoneID);
		r = Ldd2.AllocateFixed(allocSize);
		
		TESTDEBUG(test.Printf(_L("r = %d\n"), r));
		
		GetAllPageInfo();
		if (r != KErrNone)
			{
			test.Printf(_L("Fail: Fixed pages have not been allocated, r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(Ldd.FreeAllFixedPages());
			CLEANUP(Ldd2.FreeAllFixedPages());
			TEST_FAIL;
			}
		else
			{
			test.Printf(_L("Passed...\n"));
			}
		TESTDEBUG(test.Printf(_L("Freeing all Fixed Pages.....\n")));
		Ldd2.FreeAllFixedPages();
		}
	TestEnd();

	test.Next(_L("Test4: Filling the FS Cache and allocating less than 16 contiguous fixed pages"));	
	TestStart();
	
	r = FillCache(gFilesNeededToFillCache, KDefaultCacheSize);	
	if (r != KErrNone)
		{
		test.Printf(_L("File system caching failed - Skipping all file caching tests...\n"));
		goto skipFileCacheTests;
		}
	allocSize = 14 << gPageShift; 
	
	TESTDEBUG(test.Printf(_L("Filling the remaining free pages with fixed pages.....\n")));
	GetAllPageInfo();
	// Allocate the fixed array before getting any page counts
	for (TUint index = 0; index < gZoneCount; index++)
		{
		GetAllPageInfo();
		if (gZoneUtilArray[index].iFreePages)
			{
			r = Ldd.ZoneAllocToManyArray(index, gZoneUtilArray[index].iFreePages);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to allocate fixed array Zone %d r = %d - Skipping...\n"), index,  r);
				goto SkipTest4;
				}
			}
		}

	// Now fill all zones with fixed pages, 1 zone at a time
	// to avoid the discardable pages being disturbed
	for (TUint index = 0; index < gZoneCount; index++)
		{
		GetAllPageInfo();
		if (gZoneUtilArray[index].iFreePages)
			{
			r = Ldd.ZoneAllocToMany2(index, gZoneUtilArray[index].iFreePages);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to allocate %d fixed to Zone %d r = %d - Skipping...\n"), 
													gZoneUtilArray[index].iFreePages, index,  r);
				goto SkipTest4;
				}
			}
		}
	
	GetAllPageInfo();
	test.Printf(_L("number of free pages = 0x%x\n"), gTotalPageCount.iFreePages);
	if (gTotalPageCount.iFreePages || 
		gTotalPageCount.iDiscardablePages <= (TUint)(allocSize >> gPageShift))
		{
		test.Printf(_L("Setup failed - Skipping...\n"));
		goto SkipTest4;
		}

	test.Printf(_L("Allocating 0x%x fixed pages.....\n"), allocSize >> gPageShift);
	r = Ldd2.AllocContiguous(allocSize);
	TESTDEBUG(test.Printf(_L("r = %d\n"), r));
	
	GetAllPageInfo();
	if (r != KErrNone || !gTotalPageCount.iDiscardablePages)
		{
		test.Printf(_L("Fail: Fixed pages have not been allocated, r = %d, expected = %d\n"), r, KErrNone);
		CLEANUP(Ldd.FreeAllFixedPages());
		CLEANUP(Ldd2.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));
		}

SkipTest4:
	TESTDEBUG(test.Printf(_L("Freeing all Fixed Pages.....\n")));
	Ldd2.FreeAllFixedPages();
	TestEnd();

	test.Next(_L("Test5: Filling the FS Cache and allocating more than 16 contiguous fixed pages"));	
	TestStart();

	if (gMemModel >= EMemModelTypeFlexible)
		{// The flexible memory model won't flush the whole paging cache for 
		// contiguous allocations >16 pages so skip the next test.
		test.Printf(_L("This memory model won't flush the cache - Skipping...\n"));
		goto SkipTest5;
		}

	// TestEnd() will have reduced any cache pages to minimum so just get current 
	// count of discardable pages.
	GetAllPageInfo();
	minDiscardPages = gTotalPageCount.iDiscardablePages;
	
	r = FillCache(gFilesNeededToFillCache, KDefaultCacheSize);
	if (r != KErrNone)
		{
		test.Printf(_L("File system caching failed - Skipping all file caching tests...\n"));
		goto skipFileCacheTests;
		}
	
	allocSize = 18 << gPageShift; 

	TESTDEBUG(test.Printf(_L("Filling the remaining free pages with fixed pages.....\n")));
	GetAllPageInfo();
	
	// Allocate the fixed array before getting any page counts
	for (TUint index = 0; index < gZoneCount; index++)
		{
		GetAllPageInfo();
		if (gZoneUtilArray[index].iFreePages)
			{
			r = Ldd.ZoneAllocToManyArray(index, gZoneUtilArray[index].iFreePages);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to allocate fixed array Zone %d r = %d - Skipping...\n"), index,  r);
				goto SkipTest5;
				}
			}
		}

	// Now fill all zones with fixed pages, 1 zone at a time
	// to avoid the discardable pages being disturbed
	for (TUint index = 0; index < gZoneCount; index++)
		{
		GetAllPageInfo();
		if (gZoneUtilArray[index].iFreePages)
			{
			r = Ldd.ZoneAllocToMany2(index, gZoneUtilArray[index].iFreePages);
			if (r != KErrNone)
				{
				test.Printf(_L("Failed to allocate %d fixed to Zone %d r = %d - Skipping...\n"), 
													gZoneUtilArray[index].iFreePages, index,  r);
				goto SkipTest5;
				}
			}
		}
	
	GetAllPageInfo();
	test.Printf(_L("number of free pages = 0x%x\n"), gTotalPageCount.iFreePages);
	if (gTotalPageCount.iFreePages)
		{
		test.Printf(_L("Setup failed - Skipping...\n"));
		goto SkipTest5;
		}
	
	test.Printf(_L("Allocating 0x%x fixed pages.....\n"), allocSize >> gPageShift);
	r = Ldd2.AllocContiguous(allocSize);
	TESTDEBUG(test.Printf(_L("r = %d\n"), r));

	GetAllPageInfo();
	if (r != KErrNone || gTotalPageCount.iDiscardablePages != minDiscardPages)
		{
		test.Printf(_L("Fail: r = %d, expected = %d - Discardable Pages = 0x%x, expected = %d\n"), 
								r, KErrNone, gTotalPageCount.iDiscardablePages, minDiscardPages);
		CLEANUP(Ldd.FreeAllFixedPages());
		CLEANUP(Ldd2.FreeAllFixedPages());
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed...\n"));
		}
SkipTest5:

skipFileCacheTests:
	TestEnd();
	TESTDEBUG(test.Printf(_L("Freeing all Fixed Pages.....\n")));
	Ldd2.FreeAllFixedPages();
	Ldd2.Close();
	gTheFs.Close();
	FormatMMC();
	test.End();
	return KErrNone;

	}


//
// TestOneZoneConfig
//
//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-t_ramdefrag-0600
//! @SYMTestType				CIT
//! @SYMTestCaseDesc			Verifying that when only 1 zone is cofigured in the variant, that
//!								the defrag and allocation of fixed pages happend correctly
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1.	Fragment the memory and perform a DefragRam
//! 	2.	Fragment the memory and perform an EmptyZone
//! 	3.	Fragment the memory and perform a ClaimZone
//! 	4.	Call ZoneAllocPhysicalRam to allocate discontiguous fixed pages
//! 	5.	Call ZoneAllocPhysicalRam to allocate contiguous fixed pages
//!
//! @SYMTestExpectedResults
//! 	1.	Number of pages does not differ from the original
//! 	2.	KErrNoMemroy and discardable pages are discarded of
//! 	3.	KErrNoMemory
//! 	4.	KErrNone
//! 	5.	KErrNone
//---------------------------------------------------------------------------------------------------------------------
TInt TestOneZoneConfig()
	{
	TInt r = gTheFs.Connect();
	test_KErrNone(r);
	
	if (gDrive != KNoRemovableDrive)
		{	
		SetUpMMC();
		}

	TUint index = 0;
	GetAllPageInfo();
	TUint zoneID = gZoneConfigArray[index].iZoneId;
	test.Printf(_L("Zone ID = 0x%x\n"), zoneID);
	TUint minCacheSize = 0;
	TUint maxCacheSize = 0;
	TUint currentCacheSize = 0;
	

	TUint origFree = 0;
	TUint origUnknown = 0;
	TUint origFixed = 0;
	TUint origMovable = 0;
	TUint origDiscard = 0;
	TUint origOther = 0;

	if (gPagedRom)
		{
		DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
		TESTDEBUG(test.Printf(_L("Original CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
									minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));

		TUint setMin = 60 << gPageShift;
		TInt setMax = -1;
		TInt r = DPTest::SetCacheSize(setMin, setMax);
		if (r != KErrNone)
			{
			test.Printf(_L("r = %d, expected = %d\n"), r, KErrNone);
			CLEANUP(ResetDPCache());
			TEST_FAIL;
			}
		DPTest::CacheSize(minCacheSize,maxCacheSize,currentCacheSize);
		TESTDEBUG(test.Printf(_L("Original CacheSize: minCacheSize = 0x%x, maxCacheSize = 0x%x, currentCacheSize = 0x%x\n"), 
									minCacheSize >> gPageShift, maxCacheSize >> gPageShift, currentCacheSize >> gPageShift));
		}

	test.Start(_L("Test1: Fragmenting the memory and performing a general defrag"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	if (gDrive != KNoRemovableDrive)
		{
		FillCache(gFilesNeededToFillCache, KDefaultCacheSize);
		}
	else
		{
		test.Printf(_L("Cannot find drive to write files to - Not allocating discardable pages through FS Caching\n"));
		}

	GetAllPageInfo();
	origFree = gTotalPageCount.iFreePages;
	origUnknown = gTotalPageCount.iUnknownPages;
	origFixed = gTotalPageCount.iFixedPages;
	origMovable = gTotalPageCount.iMovablePages;
	origDiscard = gTotalPageCount.iDiscardablePages;
	origOther = gTotalPageCount.iOtherPages;
	
	r = Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
	
	GetAllPageInfo();

	if ((origUnknown != gTotalPageCount.iUnknownPages) ||
		(origFixed != gTotalPageCount.iFixedPages) ||
		(origMovable != gTotalPageCount.iMovablePages) ||
		(origDiscard != gTotalPageCount.iDiscardablePages) ||
		(origOther != gTotalPageCount.iOtherPages))
		{
		test.Printf(_L("Fail: Pages after defrag are not equal to those before"));
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed..."));
		}
	// This will free any allocated memory
	TestEnd();


	test.Next(_L("Test2: Fragmenting the memory and performing an EmptyZone"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	if (gDrive != KNoRemovableDrive)
		{
		FillCache(gFilesNeededToFillCache, KDefaultCacheSize);
		}
	else
		{
		test.Printf(_L("Cannot find drive to write files to - Not allocating discardable pages through FS Caching\n"));
		}	
	r = Ldd.CallDefrag(DEFRAG_TYPE_EMPTY, DEFRAG_VER_SYNC, zoneID);

	if (r != KErrNoMemory || CheckZoneIsOff(index))
		{
		test.Printf(_L("Fail: r = %d, expected = -4"), r);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed..."));
		}
	// This will free any allocated memory
	TestEnd();

	test.Next(_L("Test3: Fragmenting the memory and performing a ClaimZone"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	if (gDrive != KNoRemovableDrive)
		{
		FillCache(gFilesNeededToFillCache, KDefaultCacheSize);
		}
	else
		{
		test.Printf(_L("Cannot find drive to write files to - Not allocating discardable pages through FS Caching\n"));
		}
	
	GetAllPageInfo();
	origMovable = gTotalPageCount.iMovablePages;
		
	r = Ldd.CallDefrag(DEFRAG_TYPE_CLAIM, DEFRAG_VER_SYNC, zoneID);

	GetAllPageInfo();
	if (r != KErrNoMemory || origMovable != gTotalPageCount.iMovablePages)
		{
		test.Printf(_L("Fail: r = %d, expected = -4"), r);
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed..."));
		}
	// This will free any allocated memory.
	TestEnd();

	test.Next(_L("Test4: Calling ZoneAllocPhysicalRam to allocate discontiguous fixed pages"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);
	
	GetAllPageInfo();	
	origFree = gTotalPageCount.iFreePages;
	origUnknown = gTotalPageCount.iUnknownPages;
	origFixed = gTotalPageCount.iFixedPages;
	origMovable = gTotalPageCount.iMovablePages;
	origDiscard = gTotalPageCount.iDiscardablePages;
	origOther = gTotalPageCount.iOtherPages;
	
	r = Ldd.ZoneAllocDiscontiguous(zoneID, (TInt)(origFree / 2));

	GetAllPageInfo();
	if (gTotalPageCount.iFixedPages < (origFixed + (origFree / 2)))
		{
		test.Printf(_L("Fail: fixed pages = 0x%x, expected >= 0x%x\n"), 
						gTotalPageCount.iFixedPages, (origFixed + (origFree / 2)));
		CLEANUP(Ldd.FreeAllFixedPages());
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed..."));
		}
	Ldd.FreeAllFixedPages();
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();


	test.Next(_L("Test5: Calling ZoneAllocPhysicalRam to allocate contiguous fixed pages"));	
	TestStart();	
	
	AllocMovable(gChunkArray1, gChunkArraySize1, KNumAllocChunks);
	FreeMovable(gChunkArray1, gChunkArraySize1);

	GetAllPageInfo();
	origFree = gTotalPageCount.iFreePages;
	origUnknown = gTotalPageCount.iUnknownPages;
	origFixed = gTotalPageCount.iFixedPages;
	origMovable = gTotalPageCount.iMovablePages;
	origDiscard = gTotalPageCount.iDiscardablePages;
	origOther = gTotalPageCount.iOtherPages;
	
	TInt allocSize = 50 << gPageShift; 
	
	r = Ldd.ZoneAllocContiguous(zoneID, allocSize);

	GetAllPageInfo();
	if (gTotalPageCount.iFixedPages < (origFixed + (allocSize >> gPageShift)))
		{
		test.Printf(_L("Fail: fixed pages = 0x%x, expected >= 0x%x\n"), 
							gTotalPageCount.iFixedPages, (origFixed + (allocSize >> gPageShift)));
		CLEANUP(Ldd.FreeAllFixedPages());
		CLEANUP(RemoveChunkAlloc(gChunkArray1, gChunkArraySize1));
		TEST_FAIL;
		}
	else
		{
		test.Printf(_L("Passed..."));
		}
	Ldd.FreeAllFixedPages();
	RemoveChunkAlloc(gChunkArray1, gChunkArraySize1);
	TestEnd();

	if (gPagedRom)
		{
		test_KErrNone(DPTest::FlushCache());
		ResetDPCache();
		}

	gTheFs.Close();
	if (gDrive != KNoRemovableDrive)
		{
		FormatMMC();
		}
	
	test.End();
	return KErrNone;
	}


//
// RunDefragTests
//
// List of defrag tests to be run 
//
void RunDefragTests()
	{
	test.Start(_L("Testing the moving of pages in a defrag"));
	TestMovPgsDefrag();

	test.Next(_L("Verifying the implementation of the function TRamDefragRequest::DefragRam() arg aMaxPages"));
	TestDefragRamMaxPages();

	test.Next(_L("Verifying the implementation of the function TRamDefragRequest::EmptyRamZone()"));
	TestEmptyRamZone();

	test.Next(_L("Verifying the implementation of the function Epoc::GetRamZonePageCount()"));
	TestGetRamZonePageCount();

	test.Next(_L("Verifying the implementation of the function TRamDefragRequest::ClaimRamZone()"));
	TestClaimRamZone();

	test.Next(_L("Verifying the implementation of the function TRamDefragRequest::Cancel()"));
	TestCancelDefrag();

	test.Next(_L("Verifying that pages are moved correctly"));
	TestMovingPages();

	test.Next(_L("Verifying Semaphore Methods of the Defrag"));
	TestDefragSemMethod();

	test.Next(_L("Verifying Dfc Methods of the Defrag"));
	TestDefragDfcMethod();

	test.Next(_L("Testing priorities"));
	TestPriorities();

	test.Next(_L("Testing File System Caching"));
	if (!gPagedRom)
		{
		TestFileCaching();
		}
	else
		{
		test.Printf(_L("Skipping... \n"));
		}
	
	test.Next(_L("Testing general RAM defrag implementation"));
	TestGenDefrag();

	test.End();
	}


//
// RunAllocTests
//
// List of allocating tests to be run 
// These tests only need to be executed once
//
void RunAllocTests()
	{
	test.Start(_L("Verifying the allocating strategies"));
	TestAllocStrategies();

	test.Next(_L("Verifying the contiguous overload of Epoc::ZoneAllocPhysicalRam()"));
	TestZoneAllocContiguous();

	test.Next(_L("Verifying the discontiguous overload of Epoc::ZoneAllocPhysicalRam()"));
	TestZoneAllocDiscontiguous();
	
	test.Next(_L("Test Free Zone"));
	TestFreeZone();

	test.Next(_L("Testing zone flags"));
	TestFlags();

	test.End();
	}


//
// E32Main
//
// Main entry point.
//
TInt E32Main()
	{
	test.Title();
	DeviceDriver(TEST_DRIVER_OPEN);
	gTotalRamLost = 0;
	
	TInt r = TestSetup();
	if (r != KErrNone)
		{
		test.Printf(_L("Test Setup failed, r = %d\n"), r);
		TestCleanup();
		return r;
		}

	if (gZoneCount == 1)
		{
		GetAllPageInfo();

		test.Start(_L("Zone Count 1..."));
		TestOneZoneConfig();
		}
	else
		{
		test.Start(_L("Running Alloc tests"));
		RunAllocTests();

		Ldd.ResetDriver();
		Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		

		test.Next(_L("Running Defrag tests"));
		RunDefragTests();

		Ldd.ResetDriver();
		Ldd.CallDefrag(DEFRAG_TYPE_GEN, DEFRAG_VER_SYNC);
		}
	

	test.Printf(_L("The total number of test steps failed = %d\n"), gTestStepFailed);
	test_Equal(KErrNone, gTestStepFailed);
	
	TestCleanup();
	
	DeviceDriver(TEST_DRIVER_CLOSE);
	test.End();
	test.Close();
	
	return 0;
	}
