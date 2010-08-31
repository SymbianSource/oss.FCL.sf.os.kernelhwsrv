// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_dircache.cpp
// 
//
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <f32fsys.h>
#include <e32test.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_chlffs.h"
#include "f32_test_utils.h"
#include "fat_utils.h"
#include "d_pagestress.h"

RTest test(_L("T_DIRCACHE"));

/*
 * This whole test execute on UDEB mode only.
 */
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

TInt gDrive=-1;
const TInt32 KDef_KLeafDirCacheSize = 32;                    // default leaf dir cache number
const TInt32 KDef_DynamicDirCacheMinInBytes = 128 << 10;     // default minimum fat dir cache size in bytes
const TInt32 KDef_DynamicDirCacheMaxInBytes = 256 << 10;     // default maximum fat dir cache size in bytes
const TInt32 KDef_MaxDynamicDirCachePageSzLog2 = 14;         // default value for directory cache single page 
                                                              //  maximal size Log2, 2^14 (16K) by default
const TInt32 KMaxThreadCount = 1;                            // the maximum number of multiple threads that can
                                                              //  access dir cache concurrently.
const TInt32 KSegmentSize = 1 << 12;                            // the smallest memory unit that Kernel manages  

template <class C>
TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
    {
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
    }

// See f32\sfile\sf_memory_man.cpp for the default value settings
const TInt  KDefaultGlobalCacheMemorySize = (8 << 10) << 10;
const TInt  KDefaultLowMemoryThreshold = 10;
//----------------------------------------------------------------------------------------------
//@SYMTestCaseID      PBASE-XXXX 
//@SYMTestType        FT
//@SYMPREQ            PREQ1885
//@SYMTestCaseDesc    Check global cache settings. The global cache should be either:
//                      1. 0 (disabled)
//                      2. no less than the sum of all per-drive settings
//----------------------------------------------------------------------------------------------
void TestGlobalSettings()
    {
    test.Next(_L("Test global cache settings"));
    // read global cache settings from estart.txt
    TGlobalCacheConfig globalCacheConfig;
    TInt r = controlIo(TheFs,gDrive, KControlIoGlobalCacheConfig, globalCacheConfig);
	test_KErrNone(r);
    test_Value (globalCacheConfig.iGlobalCacheSizeInBytes,
				globalCacheConfig.iGlobalCacheSizeInBytes > 0 || globalCacheConfig.iGlobalCacheSizeInBytes == KErrNotFound);
    test_Value (globalCacheConfig.iGlobalLowMemoryThreshold,
				globalCacheConfig.iGlobalLowMemoryThreshold >= 0 || globalCacheConfig.iGlobalLowMemoryThreshold == KErrNotFound);

    const TInt32 globalCacheSize = globalCacheConfig.iGlobalCacheSizeInBytes > 0 ? 
                                       globalCacheConfig.iGlobalCacheSizeInBytes : KDefaultGlobalCacheMemorySize;

    // test if global cache is enabled, it is configured in the way that its figure is no less
    //  than the sum of per-drive max size settings (taking default values into account). 
    TInt32 sumDirCacheMaxSize = 0;
    for (TInt i = 0; i < KMaxDrives; i++)
        {
        TBuf<0x20>  fsName;
        r = TheFs.FileSystemName(fsName, i);

        if (r == KErrNone && (F32_Test_Utils::Is_Fat(TheFs, i) || F32_Test_Utils::Is_ExFat(TheFs, i)))
            {
            test.Printf(_L("drive[%C:] file system: (\"%S\")\n"), 'A' + i, &fsName);
            TDirCacheConfig dirCacheConfig;
            r = controlIo(TheFs,gDrive, KControlIoDirCacheConfig, dirCacheConfig);
            test_KErrNone(r);
            if(dirCacheConfig.iDirCacheSizeMax > 0)
                {
                sumDirCacheMaxSize += (dirCacheConfig.iDirCacheSizeMax > KDef_DynamicDirCacheMaxInBytes ? dirCacheConfig.iDirCacheSizeMax : KDef_DynamicDirCacheMaxInBytes);
                }
            else
                {
                sumDirCacheMaxSize += KDef_DynamicDirCacheMaxInBytes;
                }
            test.Printf(_L("++sumDirCacheMaxSize = %d\n"), sumDirCacheMaxSize);
            }
        }
    test_Compare(globalCacheSize, >=, sumDirCacheMaxSize);
    }

//----------------------------------------------------------------------------------------------
//@SYMTestCaseID      PBASE-XXXX 
//@SYMTestType        FT
//@SYMPREQ            PREQ1885
//@SYMTestCaseDesc    Test current drive's dir cache configurations, the current dir cache info should
//                      match the configurations read from estart.txt file.
//----------------------------------------------------------------------------------------------
void TestDirCacheSettings()
    {
    test.Next(_L("Test current drive's dir cache settings"));
    
    // test global cache config is ON 
    TGlobalCacheConfig globalCacheConfig;
    TInt r = controlIo(TheFs,gDrive, KControlIoGlobalCacheConfig, globalCacheConfig);
    test_KErrNone(r);
    test_Value (globalCacheConfig.iGlobalCacheSizeInBytes,
				globalCacheConfig.iGlobalCacheSizeInBytes > 0 || globalCacheConfig.iGlobalCacheSizeInBytes == KErrNotFound);
    test_Value (globalCacheConfig.iGlobalLowMemoryThreshold,
				globalCacheConfig.iGlobalLowMemoryThreshold >= 0 || globalCacheConfig.iGlobalLowMemoryThreshold == KErrNotFound);
    
    // test global cache info is corresponding to the configurations
    TGlobalCacheInfo globalCacheInfo;
    r = controlIo(TheFs,gDrive, KControlIoGlobalCacheInfo, globalCacheInfo);
    test_KErrNone(r);

    if (globalCacheConfig.iGlobalCacheSizeInBytes == KErrNotFound)
        {
        test_Equal(KDefaultGlobalCacheMemorySize, globalCacheInfo.iGlobalCacheSizeInBytes);
        }
    else
        {
        test_Equal(globalCacheConfig.iGlobalCacheSizeInBytes, globalCacheInfo.iGlobalCacheSizeInBytes);
        }

    if (globalCacheConfig.iGlobalLowMemoryThreshold == KErrNotFound)
        {
        test_Equal(KDefaultLowMemoryThreshold, globalCacheInfo.iGlobalLowMemoryThreshold);
        }
    else
        {
        test_Equal(globalCacheConfig.iGlobalLowMemoryThreshold, globalCacheInfo.iGlobalLowMemoryThreshold);
        }
    
    // read per-drive settings from estart.txt
    TDirCacheConfig dirCacheConfig;
    r = controlIo(TheFs,gDrive, KControlIoDirCacheConfig, dirCacheConfig);
    test_KErrNone(r);
    test_Value (dirCacheConfig.iLeafDirCacheSize,
				dirCacheConfig.iLeafDirCacheSize >= 0 || dirCacheConfig.iLeafDirCacheSize == KErrNotFound);
    test_Value (dirCacheConfig.iDirCacheSizeMin,
				dirCacheConfig.iDirCacheSizeMin >= 0 || dirCacheConfig.iDirCacheSizeMin == KErrNotFound);
    test_Value (dirCacheConfig.iDirCacheSizeMax,
				dirCacheConfig.iDirCacheSizeMax >= 0 || dirCacheConfig.iDirCacheSizeMax == KErrNotFound);
    
    // caculate expected settings according to the readings from estart.txt
    if (dirCacheConfig.iLeafDirCacheSize == 0)
        dirCacheConfig.iLeafDirCacheSize = 1;
    if (dirCacheConfig.iLeafDirCacheSize == KErrNotFound)
        dirCacheConfig.iLeafDirCacheSize = KDef_KLeafDirCacheSize;
    if (dirCacheConfig.iDirCacheSizeMin < KDef_DynamicDirCacheMinInBytes)
        dirCacheConfig.iDirCacheSizeMin = KDef_DynamicDirCacheMinInBytes;
    if (dirCacheConfig.iDirCacheSizeMax < KDef_DynamicDirCacheMaxInBytes)
        dirCacheConfig.iDirCacheSizeMax = KDef_DynamicDirCacheMaxInBytes;
    
    TVolumeIOParamInfo ioParam;
    r = TheFs.VolumeIOParam(gDrive, ioParam);
    test_KErrNone(r);
    const TInt32 KClusterSize = ioParam.iClusterSize;
    test.Printf(_L("DRV[%C:] cluster = %d\n"), gDrive + 'A', ioParam.iClusterSize); 

    const TInt32 KDefMaxCachePageSize = 1 << KDef_MaxDynamicDirCachePageSzLog2;
    const TInt32 KPageSizeInData = KClusterSize < KDefMaxCachePageSize ? KClusterSize : KDefMaxCachePageSize;
    const TInt32 KPageSizeInMem = KPageSizeInData < KSegmentSize ? KSegmentSize : KPageSizeInData;
    const TInt32 KCacheSizeMinInPages = dirCacheConfig.iDirCacheSizeMin / KPageSizeInMem;
    const TInt32 KCacheSizeMaxInPages = dirCacheConfig.iDirCacheSizeMax / KPageSizeInMem;
    const TInt32 KUnlockedPageNum = 0;
    
    // remount drive, get current dir cache info and test
    r = F32_Test_Utils::RemountFS (TheFs, CurrentDrive(), NULL);
    test_KErrNone(r);

    TDirCacheInfo dirCacheInfo;
    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);
    
    test_Equal(KSegmentSize, dirCacheInfo.iMemorySegmentSize);
    test_Equal(KPageSizeInMem, dirCacheInfo.iPageSizeInMemory);
    test_Equal(KPageSizeInData, dirCacheInfo.iPageSizeInData);
    test_Equal(KCacheSizeMinInPages, dirCacheInfo.iMinCacheSizeInPages);
    test_Equal(KCacheSizeMaxInPages, dirCacheInfo.iMaxCacheSizeInPages);
    test_Equal(KMaxThreadCount, dirCacheInfo.iLockedPageNumber);
    test_Equal(KUnlockedPageNum, dirCacheInfo.iUnlockedPageNumber);
    }

//----------------------------------------------------------------------------------------------
//@SYMTestCaseID      PBASE-XXXX 
//@SYMTestType        FT
//@SYMPREQ            PREQ1885
//@SYMTestCaseDesc    Test populating dir cache under normal memory conditions.
//----------------------------------------------------------------------------------------------
void TestPopulateCache()
    {
    test.Next(_L("Test populating dir cache under normal memory conditions"));
    
    CFileMan* fileMan = CFileMan::NewL(TheFs);
    test_NotNull(fileMan);
    TInt r = fileMan->RmDir(_L("\\TEST_DIRCACHE\\"));
    test_Value(r, r==KErrNone || r==KErrPathNotFound);

    // remount drive, get current dir cache info
    r = F32_Test_Utils::RemountFS (TheFs, CurrentDrive(), NULL);
    test_KErrNone(r);

    /*
     * Test populating dir cache 
     */
    TVolumeIOParamInfo ioParam;
    r = TheFs.VolumeIOParam(gDrive, ioParam);
    test_KErrNone(r);
    const TInt32 KClusterSize = ioParam.iClusterSize;

    TFileName dirPath = _L("\\TEST_DIRCACHE\\");
    r = TheFs.MkDirAll(dirPath);
    test_KErrNone(r);

    TDirCacheInfo dirCacheInfo;
    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);

    // this calculation is for volumes that have a large cluster size, larger than the allowed maximum
    //  page size. on this case, creating a new directory will generate two or more new pages in the 
    //  dir cache 
    const TUint pagesPerDir = KClusterSize > dirCacheInfo.iPageSizeInMemory ? 
                            KClusterSize / dirCacheInfo.iPageSizeInMemory : 1;
    
    // should be KMaxThreadCount + root dir (1 page) + "\\TEST_DIRCACHE\\" (number of pages that are 
    //  needed for new directories, i.e. pagesPerDir)
    test_Equal(KMaxThreadCount + 1 + pagesPerDir, dirCacheInfo.iLockedPageNumber);

    const TInt initialUnlockedPage = dirCacheInfo.iUnlockedPageNumber;

    const TInt createdNewDirs = dirCacheInfo.iMinCacheSizeInPages - dirCacheInfo.iLockedPageNumber;
    // create directories so that it grows to KCacheSizeMinInPages
    for (TInt i = 1; i <= createdNewDirs; i++)
        {
        dirPath = _L("\\TEST_DIRCACHE\\");
        TFileName dirName;
        dirName.Format(_L("DIR%d\\"), i);
        dirPath += dirName;
        r = TheFs.MkDirAll(dirPath);
        test_KErrNone(r);
        }
    
    const TInt KFatDirEntrySize = 32;
    const TInt subDirNum = dirCacheInfo.iMinCacheSizeInPages - dirCacheInfo.iLockedPageNumber;
    // calculate the extra pages needed for the newly created sub dir entries (plus the original 
    //  "." and ".." entries)
    TInt extraPagesForLeafdir = ((subDirNum + 2) * KFatDirEntrySize / dirCacheInfo.iPageSizeInData) - 1;
    if (((subDirNum + 2) * KFatDirEntrySize) % dirCacheInfo.iPageSizeInData > 0)
        extraPagesForLeafdir++;
    test.Printf(_L("!!Extra pages needed for leafdir = %d\n"), extraPagesForLeafdir);   //kk

    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);
    r = controlIo(TheFs,gDrive, 15, dirCacheInfo);
    test_KErrNone(r);

    test_Equal(dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iLockedPageNumber);
    
    const TInt maxUnlockedPage = dirCacheInfo.iMaxCacheSizeInPages - dirCacheInfo.iMinCacheSizeInPages;
    // calculating the expected unlocked page number here.
    TInt newUnlockedPage = initialUnlockedPage;
    newUnlockedPage += extraPagesForLeafdir;                // if any extra pages are created for the leafdir
    newUnlockedPage += createdNewDirs * (pagesPerDir - 1);   // if more than one page is needed for each dir creation 
    test_Equal((newUnlockedPage > maxUnlockedPage ? maxUnlockedPage : newUnlockedPage), dirCacheInfo.iUnlockedPageNumber);

    // create directories so that it grows to KCacheSizeMinInPages + KCacheSizeMaxInPages
    if (dirCacheInfo.iMaxCacheSizeInPages > dirCacheInfo.iMinCacheSizeInPages)
        {
        for (TInt i = 1; i <= dirCacheInfo.iMaxCacheSizeInPages - dirCacheInfo.iMinCacheSizeInPages; i++)
            {
            dirPath = _L("\\TEST_DIRCACHE\\");
            TFileName dirName;
            dirName.Format(_L("DIR_UNLOCKED%d\\"), i);
            dirPath += dirName;
            r = TheFs.MkDirAll(dirPath);
            test_KErrNone(r);
            }
        r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
        test_KErrNone(r);
        r = controlIo(TheFs,gDrive, 15, dirCacheInfo);
        test_KErrNone(r);

        test_Equal(dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iLockedPageNumber);
        
        test_Equal(dirCacheInfo.iMaxCacheSizeInPages - dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iUnlockedPageNumber);
        }
    
    // create more directories and check that dir cache doesn't grow beyond KCacheSizeMaxInPages
    for (TInt j = 1; j <= dirCacheInfo.iMinCacheSizeInPages; j++)
        {
        dirPath = _L("\\TEST_DIRCACHE\\");
        TFileName dirName;
        dirName.Format(_L("DIR_MORE%d\\"), j);
        dirPath += dirName;
        r = TheFs.MkDirAll(dirPath);
        test_KErrNone(r);
        }
    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);
    r = controlIo(TheFs,gDrive, 15, dirCacheInfo);
    test_KErrNone(r);

    test_Equal(dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iLockedPageNumber);
    test_Equal(dirCacheInfo.iMaxCacheSizeInPages - dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iUnlockedPageNumber);

    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);

    r = fileMan->RmDir(_L("\\TEST_DIRCACHE\\"));
    test_KErrNone(r);
    
    delete fileMan;
    }

//----------------------------------------------------------------------------------------------
//@SYMTestCaseID      PBASE-XXXX 
//@SYMTestType        FT
//@SYMPREQ            PREQ1885
//@SYMTestCaseDesc    Test simulating low memory condition.
//                    1. The cache should stop growing when it is at its minimum cache size in pages
//                    2. The cache should function properly when it stops simulating low memory condition 
//----------------------------------------------------------------------------------------------
void TestSimulatingLowMemory()
    {
    // remount drive, get current dir cache info and test
    test.Next(_L("Test dir cache growth when simulating low memory condition"));

    CFileMan* fileMan = CFileMan::NewL(TheFs);
    test(fileMan != NULL);
    TInt r = fileMan->RmDir(_L("\\TEST_DIRCACHE\\"));
    test_Value(r, r==KErrNone || r==KErrPathNotFound);

    r = F32_Test_Utils::RemountFS (TheFs, CurrentDrive(), NULL);
    test_KErrNone(r);

    r = TheFs.ControlIo(gDrive, KControlIoSimulateMemoryLow);
    test_KErrNone(r);

    /*
     * Test populating dir cache 
     */
    TFileName dirPath = _L("\\TEST_DIRCACHE\\");
    r = TheFs.MkDirAll(dirPath);
    test_KErrNone(r);

    TDirCacheInfo dirCacheInfo;
    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);

    // create directories so that it grows to KCacheSizeMaxInPages
    for (TInt i = 1; i <= dirCacheInfo.iMaxCacheSizeInPages; i++)
        {
        dirPath = _L("\\TEST_DIRCACHE\\");
        TFileName dirName;
        dirName.Format(_L("DIR%d\\"), i);
        dirPath += dirName;
        r = TheFs.MkDirAll(dirPath);
        test_KErrNone(r);
        }

    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);

    test_Equal(dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iLockedPageNumber);
    test_Equal(0, dirCacheInfo.iUnlockedPageNumber);
    
    r = fileMan->RmDir(_L("\\TEST_DIRCACHE\\"));
    test_KErrNone(r);
 
    // test when stop simulating the low memory condition, cache grows normally
    test.Next(_L("Test dir cache growth when stop simulating low memory condition"));
    r = TheFs.ControlIo(gDrive, KControlIoStopSimulateMemoryLow);
    test_KErrNone(r);

    r = F32_Test_Utils::RemountFS (TheFs, CurrentDrive(), NULL);
    test_KErrNone(r);

    /*
     * Test populating dir cache 
     */
    dirPath = _L("\\TEST_DIRCACHE\\");
    r = TheFs.MkDirAll(dirPath);
    test_KErrNone(r);

    // create directories so that it grows to KCacheSizeMaxInPages
    for (TInt j = 1; j <= dirCacheInfo.iMaxCacheSizeInPages; j++)
        {
        dirPath = _L("\\TEST_DIRCACHE\\");
        TFileName dirName;
        dirName.Format(_L("DIR%d\\"), j);
        dirPath += dirName;
        r = TheFs.MkDirAll(dirPath);
        test_KErrNone(r);
        }

    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);

    test_Equal(dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iLockedPageNumber);
    test_Equal(dirCacheInfo.iMaxCacheSizeInPages - dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iUnlockedPageNumber);
    
    r = fileMan->RmDir(_L("\\TEST_DIRCACHE\\"));
    test_KErrNone(r);

    delete fileMan;
    }

//----------------------------------------------------------------------------------------------
//@SYMTestCaseID      PBASE-XXXX 
//@SYMTestType        FT
//@SYMPREQ            PREQ1885
//@SYMTestCaseDesc    Test low memory threshold on hardware platforms
//                    1. Stress the system memory to below the low memory threshold configured
//                    2. The cache should stop growing when it is at its minimum cache size in pages
//                    3. Resume the system memory before step 1.
//                    2. The cache should function properly when it stops simulating low memory condition 
//----------------------------------------------------------------------------------------------
void TestLowMemoryHW()
    {
    test.Next(_L("Test low memory threshold on hardware"));
#if !defined(__WINS__)

    CFileMan* fileMan = CFileMan::NewL(TheFs);
    test_NotNull(fileMan);
    TInt r = fileMan->RmDir(_L("\\TEST_DIRCACHE\\"));
    test_Value(r, r==KErrNone || r==KErrPathNotFound);

    r = F32_Test_Utils::RemountFS (TheFs, CurrentDrive(), NULL);
    test_KErrNone(r);

    RPageStressTestLdd PagestressLdd;
    TMemoryInfoV1Buf memInfo;
    r = UserHal::MemoryInfo(memInfo);
    test_KErrNone(r);

    TGlobalCacheInfo globalCacheInfo;
    r = controlIo(TheFs,gDrive, KControlIoGlobalCacheInfo, globalCacheInfo);
    test_KErrNone(r);

    test.Printf(_L("Free RAM before setup %d\n"), memInfo().iFreeRamInBytes);
    const TReal lowMemThreshold = globalCacheInfo.iGlobalLowMemoryThreshold / 200.00;
    test.Printf(_L("Low memory threshold = %d%%\n"), globalCacheInfo.iGlobalLowMemoryThreshold);
    test.Printf(_L("Current memory available = %d%%\n"), lowMemThreshold * 100);

    const TInt totalRamInBytes = memInfo().iTotalRamInBytes;
    const TInt setupFreeRamInBytes = (TInt) (lowMemThreshold * totalRamInBytes);
    const TInt setupFreeRamInPages = setupFreeRamInBytes / KSegmentSize;
    
    r = User::LoadLogicalDevice(KPageStressTestLddName);
    test_Value(r, r==KErrNone || r==KErrAlreadyExists);
    r = PagestressLdd.Open();
    test_KErrNone(r);
    r = PagestressLdd.DoSetDebugFlag((TInt)ETrue);
    test_KErrNone(r);
    if (setupFreeRamInPages > 0 && setupFreeRamInBytes < totalRamInBytes)
        {
        r = PagestressLdd.DoConsumeRamSetup(setupFreeRamInPages, 1);
        test_KErrNone(r);
        r = UserHal::MemoryInfo(memInfo);
        test_KErrNone(r);
        test.Printf(_L("Free RAM after setup %d\n"), memInfo().iFreeRamInBytes);
        }
    else
        {
        test.Printf(_L("Current memory is already low: %d\n"), memInfo().iFreeRamInBytes);
        }

    /*
     * Test populating dir cache 
     */
    TFileName dirPath = _L("\\TEST_DIRCACHE\\");
    r = TheFs.MkDirAll(dirPath);
    test_KErrNone(r);

    TDirCacheInfo dirCacheInfo;
    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);

    // create directories so that it grows to KCacheSizeMaxInPages
    for (TInt i = 1; i <= dirCacheInfo.iMaxCacheSizeInPages; i++)
        {
        dirPath = _L("\\TEST_DIRCACHE\\");
        TFileName dirName;
        dirName.Format(_L("DIR%d\\"), i);
        dirPath += dirName;
        r = TheFs.MkDirAll(dirPath);
        test_KErrNone(r);
        }

    r = controlIo(TheFs,gDrive, KControlIoDirCacheInfo, dirCacheInfo);
    test_KErrNone(r);

    test_Equal(dirCacheInfo.iMinCacheSizeInPages, dirCacheInfo.iLockedPageNumber);
    test_Equal(0, dirCacheInfo.iUnlockedPageNumber);
    
    // release memory
    PagestressLdd.DoConsumeRamFinish();
    PagestressLdd.Close();
    r = User::FreeLogicalDevice(KPageStressTestLddName);
    test_KErrNone(r);
    
    r = UserHal::MemoryInfo(memInfo);
    test_KErrNone(r);
    test.Printf(_L("Free RAM after test %d\n"), memInfo().iFreeRamInBytes);

    r = fileMan->RmDir(_L("\\TEST_DIRCACHE\\"));
    test_KErrNone(r);
    delete fileMan;
#else
    test.Printf(_L("This test step only runs on hardware!!\n"));
#endif //#if !defined(__WINS__)
    }

#endif // #if defined(_DEBUG) || defined(_DEBUG_RELEASE)

GLDEF_C void CallTestsL()
//
// Test the file server.
//
    {
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDrive);
    test_KErrNone(nRes);

    TDriveInfo  driveInfo;
    nRes = TheFs.Drive(driveInfo, gDrive);
    test.Printf(_L("MediaType: 0x%x\n"), driveInfo.iType);
    test_KErrNone(nRes);
    
    if (F32_Test_Utils::Is_Fat(TheFs, gDrive) && driveInfo.iType != EMediaRam)
        {
        nRes = F32_Test_Utils::FormatDrive(TheFs, gDrive, ETrue);
        test_KErrNone(nRes);
        
        F32_Test_Utils::PrintDrvInfo(TheFs, gDrive);
        
        TVolumeInfo v;
        TInt r=TheFs.Volume(v, CurrentDrive());
        test_KErrNone(r);
        
        CreateTestDirectory(_L("\\F32-TST\\TDIRCACHE\\"));
        
        TestGlobalSettings();
        TestDirCacheSettings();
        TestPopulateCache();
        TestSimulatingLowMemory();
        TestLowMemoryHW();
        
        DeleteTestDirectory();
        }
    else
        {
        test.Printf(_L("This test executes on FAT drives and non-RAM media only!!\n"));
        }
#else
    test.Printf(_L("This test executes on DEBUG mode only!!\n"));
#endif //#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
    }

/*
void TestReadEstart()
    {
    TDirCacheConfig dirCacheConfig;
    TInt r = controlIo(TheFs,gDrive, KControlIoDirCacheConfig, dirCacheConfig);
    test_KErrNone(r);
    test.Printf(_L("Dir cache: \n Drive %C \n iLeafDirCacheSize %d \n iDirCacheSizeMin %d \n iDirCacheSizeMax %d \n iGlobalCacheMemorySize %d \n iLowMemoryThreshold %%%d"), 
            dirCacheConfig.iDrive + 'A',
            dirCacheConfig.iLeafDirCacheSize,
            dirCacheConfig.iDirCacheSizeMin,
            dirCacheConfig.iDirCacheSizeMax);
    }

*/

//          const TInt KControlIoGlobalCacheConfig=KMaxTInt-21;
//          const TInt KControlIoGlobalCacheInfo=KMaxTInt-22;
//          const TInt KControlIoDirCacheConfig=KMaxTInt-23;
//          const TInt KControlIoDirCacheInfo=KMaxTInt-24;
//
//class TGlobalCacheConfig
//class TGlobalCacheInfo
//class TDirCacheConfig
//class TDirCacheInfo 

//  this test is for FAT only!!
//if (F32_Test_Utils::Is_ExFat(TheFs, gDrive))
//    {
//    // test cluster size
//    TVolumeIOParamInfo ioParam;
//    TInt r = TheFs.VolumeIOParam(gDrive, ioParam);
//    test.Printf(_L("TheFs.VolumeIOParam for EXFAT: (cluster = %d) = %d\n"), ioParam.iClusterSize, r); 
//    }
//
