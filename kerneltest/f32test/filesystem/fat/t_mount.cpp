// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_mount.cpp
// Testing some issues connected with mounting/dismounting file fystems, drives finalisation etc.
//
//

/**
 @file
*/

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <e32property.h>
#include <f32dbg.h>

#include "t_server.h"
#include "fat_utils.h"

using namespace Fat_Test_Utils;

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

RTest test(_L("T_Mount"));

static TInt     gDriveNum=-1; ///< drive number we are dealing with
static TInt64   gRndSeed;


const TInt KBM_Repetitions = 5;     ///< number of repetitions for BM testing
const TUint32 KCheckFileSize = 533; ///< size of the small file to be deleted

_LIT(KFileNameFirst,  "\\FIRST%d.DAT");
_LIT(KFileNameMiddle, "\\MID%d.DAT");
_LIT(KFileNameLast,   "\\LAST%d.DAT");
_LIT(KFileNameFiller, "\\FILL%d.DAT");

typedef void (*TStressFN)(TInt);    //-- pointer to the FAT mount stress function


//-------------------------------------------------------------------
//-- debug bit flags that may be set in the property which controls FAT volume mounting

const TUid KThisTestSID={0x10210EB3}; ///< this EXE SID

const TUint32 KMntProp_EnableALL    = 0x00000000; //-- enable all operations

#ifdef _DEBUG

const TUint32 KMntProp_DisableALL   = 0xFFFFFFFF; //-- disable all operations
const TUint32 KMntProp_Disable_FsInfo       = 0x00000001; //-- mask for disabling/enabling FSInfo information
const TUint32 KMntProp_Disable_FatBkGndScan = 0x00000002; //-- mask for disabling/enabling FAT background scanner

#endif

//-------------------------------------------------------------------

/** set a debug property value, which then wiil be read by FAT fsy */
void SetFsyDebugFlag(TInt aDriveNo, TUint32 aFlags)
{
    TInt nRes;

    nRes = RProperty::Set(KThisTestSID, aDriveNo, aFlags);
    test_KErrNone(nRes);
}

//-------------------------------------------------------------------
/**
    format the volume and read the boot sector
*/
static void FormatVolume(TBool aQuickFormat)
{
    TInt nRes;

    #if 0
    //-- FAT32 SPC:1; for the FAT32 testing on the emulator
    (void)aQuickFormat;

    #ifdef  __EPOC32__
    test.Printf(_L("This is emulator configuration!!!!\n"));
    test(0);
    #endif

    TFatFormatParam fp;
    fp.iFatType = EFat32;
    fp.iSecPerCluster = 1;
    nRes = FormatFatDrive(TheFs, CurrentDrive(), ETrue, &fp); //-- always quick; doesn't matter for the emulator
    #else
    nRes = FormatFatDrive(TheFs, CurrentDrive(), aQuickFormat);
    #endif

    test_KErrNone(nRes);

}

//-------------------------------------------------------------------

/**
    Prepare FAT volume for mounting performance testing

    1. quick format the drive
    2. create KBM_Repetitions files in the beginning (first files)
    3. create KMaxFillFiles/2 large files (they take about 40% of the volume);
    4. create KBM_Repetitions files (middle files)
    5. create KMaxFillFiles/2 large files (they take other 40% of the volume);
    6. create KBM_Repetitions files (last files)

    @return ETrue if everythis is OK
*/
TBool PrepareVolumeForBM()
{
    test.Printf(_L("Prepare the volume for BM testing...\n"));

    TInt nRes;
    TInt i;

    //-- 1. quick format the drive
    FormatVolume(ETrue);


    if(!Is_Fat32(TheFs, gDriveNum))
    {
        test.Printf(_L("This test requires FAT32 ! Skipping.\n"));
        return EFalse;
    }

    TVolumeInfo volInfo;
    nRes = TheFs.Volume(volInfo, gDriveNum);
    test_KErrNone(nRes);

    //-- the files will take 80% of the drive space
    const TInt    KMaxFillFiles = 100;
    const TUint32 KFillFileSz = (TUint32)((volInfo.iFree*8) / (10*KMaxFillFiles));

    //-- 2. create KBM_Repetitions files in the very begining (occupies first FAT entries)
    TBuf<64> buf;

    for(i=0; i<KBM_Repetitions; ++i)
    {
        buf.Format(KFileNameFirst, i);
        nRes = CreateCheckableStuffedFile(TheFs, buf, KCheckFileSize);
        test_KErrNone(nRes);
    }

    //-- 3. Fill the FAT with entries and cteate a file in the middle
    const TInt nHalf1 = KMaxFillFiles / 2;

    for(i=0; i<nHalf1; ++i)
    {
        buf.Format(KFileNameFiller, i);
        nRes = CreateEmptyFile(TheFs, buf, KFillFileSz);
        test_KErrNone(nRes);
    }

    //-- 4. create a files in the middle
    for(i=0; i<KBM_Repetitions; ++i)
    {
        buf.Format(KFileNameMiddle, i);
        nRes = CreateCheckableStuffedFile(TheFs, buf, KCheckFileSize);
        test_KErrNone(nRes);
    }

    //-- 5. fill second half FAT
    for(i=nHalf1; i<KMaxFillFiles; ++i)
    {
        buf.Format(KFileNameFiller, i);
        nRes = CreateEmptyFile(TheFs, buf, KFillFileSz);
        test_KErrNone(nRes);
    }


    //-- 6. create files in the very end (occupiy last FAT entries)
    for(i=0; i<KBM_Repetitions; ++i)
    {
        buf.Format(KFileNameLast, i);
        nRes = CreateCheckableStuffedFile(TheFs, buf, KCheckFileSize);
        test_KErrNone(nRes);
    }

    return ETrue;
}

/**
    Mounts and dismounts FAT volume several times calculating average time taken to mount.
    Also can stress FS by calling stress function that can do some work on the volume.

    @param  apStressFN pointer to the stressing function which can be called just after mounting. Can be NULL.
    @return time in milliseconds taken to mout the volume
*/
static TUint32 DoMeasureMountTime(TStressFN apStressFN)
{

    TInt nRes;

    TTime   timeStart;
    TTime   timeEnd;

    TInt64   usMountTime=0;      //-- total time taken by "Mount"

    //-- disable FAT test utils print out, it can affects measured time
    EnablePrintOutput(EFalse);

    for(TInt i=0; i<KBM_Repetitions; ++i)
    {

        //-- A. remount FS taking the time when mounting starts
        nRes = RemountFS(TheFs, gDriveNum, &timeStart);
        test_KErrNone(nRes);

        //-- B. call a given stress function
        if(apStressFN)
            apStressFN(i);

        //-- C. take end time
        timeEnd.UniversalTime();
        test_KErrNone(nRes);

        usMountTime += (timeEnd.MicroSecondsFrom(timeStart)).Int64();
     }

    const TUint32 msMountTime = (TUint32)usMountTime / (K1mSec*KBM_Repetitions);

    EnablePrintOutput(ETrue); //-- Enable FAT test utils print out

    return msMountTime;
}

//-------------------------------------------------------------------

/**
    Stress function.
    Use case: read access to the first file on the volume in root dir.
*/
static void FirstReadAccess_FirstFile_Stress(TInt aRepNo)
{
    TBuf<64> buf;
    buf.Format(KFileNameFirst, aRepNo);

    TInt nRes = VerifyCheckableFile(TheFs, buf);
    test_KErrNone(nRes);
}


/**
    Stress function.
    Use case: read access to the middle file on the volume in root dir.
*/
static void FirstReadAccess_MiddleFile_Stress(TInt aRepNo)
{
    TBuf<64> buf;
    buf.Format(KFileNameMiddle, aRepNo);

    TInt nRes = VerifyCheckableFile(TheFs, buf);
    test_KErrNone(nRes);
}

/**
    Stress function.
    Use case: read access to the last file on the volume in root dir.
*/
static void FirstReadAccess_LastFile_Stress(TInt aRepNo)
{
    TBuf<64> buf;
    buf.Format(KFileNameLast, aRepNo);

    TInt nRes = VerifyCheckableFile(TheFs, buf);
    test_KErrNone(nRes);
}

/**
    Stress function.
    Use case: Getting volume information SYNCHRONOUSLY.
*/
static void GetVolInfo_Synch_Stress(TInt)
{
    TVolumeInfo volInfo;
    TInt nRes = TheFs.Volume(volInfo, gDriveNum);
    test_KErrNone(nRes);
}

/**
    Stress function.
    Use case: Getting volume information _ASYNCHRONOUSLY_, i.e. do not wait until
    FAT32 free space scan thread finishes
*/
static void GetVolInfo_Asynch_Stress(TInt)
{
    TVolumeInfo volInfo;

    //-- let's use special version of the RFS API
    TRequestStatus rqStat;
    TheFs.Volume(volInfo, gDriveNum, rqStat);

    User::WaitForRequest(rqStat);
    test(rqStat.Int()==KErrNone);

}

//-------------------------------------------------------------------

/**
    Stress function.
    Use case: deletes files in the beginning of the volume (or FAT table)
*/
static void DeleteFirstFile_Stress(TInt aRepNo)
{
    TBuf<64> buf;
    buf.Format(KFileNameFirst, aRepNo);

    TInt nRes = TheFs.Delete(buf);
    test_KErrNone(nRes);
}


/**
    Stress function.
    Use case: deletes files in the middle of the volume (or FAT table)
*/
static void DeleteMiddleFile_Stress(TInt aRepNo)
{
    TBuf<64> buf;
    buf.Format(KFileNameMiddle, aRepNo);

    TInt nRes = TheFs.Delete(buf);
    test_KErrNone(nRes);
}

/**
    Stress function.
    Use case: deletes files in the end of the volume (or FAT table)
*/
static void DeleteLastFile_Stress(TInt aRepNo)
{
    TBuf<64> buf;
    buf.Format(KFileNameLast, aRepNo);

    TInt nRes = TheFs.Delete(buf);
    test_KErrNone(nRes);
}


//-------------------------------------------------------------------

/**
    perform series of tests that measure and print out FAT volume mount time for different secenarios
*/
void MeasureMountTime()
{
    TUint32 msTime;

    if(!PrepareVolumeForBM()) //-- prepare the volume for BM tests: format it, create file structure etc.
        return;

    //-- 1. no stress function, measure just pure mount time
    msTime = DoMeasureMountTime(NULL);
    test.Printf(_L("#--> Pure mount:%d ms\n"), msTime);

    //-- 2.1 measure mount time with a read-acess to the first file on the volume
    msTime = DoMeasureMountTime(FirstReadAccess_FirstFile_Stress);
    test.Printf(_L("#--> mount and read access to 1st file:%d ms\n"), msTime);

    //-- 2.2 measure mount time with a read-acess to the middle file on the volume
    msTime = DoMeasureMountTime(FirstReadAccess_MiddleFile_Stress);
    test.Printf(_L("#--> mount and read access to middle file:%d ms\n"), msTime);

    //-- 2.3 measure mount time with a read-acess to the last file on the volume
    msTime = DoMeasureMountTime(FirstReadAccess_LastFile_Stress);
    test.Printf(_L("#--> mount and read access to last file:%d ms\n"), msTime);

    //-- 2.4 measure mount time with getting a volume information
    msTime = DoMeasureMountTime(GetVolInfo_Synch_Stress);
    test.Printf(_L("#--> mount and getting volInfo (synch):%d ms\n"), msTime);

    msTime = DoMeasureMountTime(GetVolInfo_Asynch_Stress);
    test.Printf(_L("#--> mount and getting volInfo (Asynch):%d ms\n"), msTime);


    //-- 2.4 measure mount time with deleting file in the beginning (write access to the first entries of the FAT32)
    msTime = DoMeasureMountTime(DeleteFirstFile_Stress);
    test.Printf(_L("#--> mount and delete first file:%d ms\n"), msTime);

    //-- 2.5 measure mount time with deleting file in the middle (write access to the middle entries of the FAT32)
    msTime = DoMeasureMountTime(DeleteMiddleFile_Stress);
    test.Printf(_L("#--> mount and delete middle file:%d ms\n"), msTime);

    //-- 2.6 measure mount time with deleting file in the end (write access to the last entries of the FAT32)
    msTime = DoMeasureMountTime(DeleteLastFile_Stress);
    test.Printf(_L("#--> mount and delete last file:%d ms\n"), msTime);

    test.Printf(_L("---\n"), msTime);
}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_MOUNT-0521
//! @SYMTestType        PT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    Testing FAT volume mount performance for various scenarios
//!
//! @SYMTestActions
//!             0   Prepare the volume by formatting it and creating 100 files to occupy the space.
//!             1   Turn OFF all mount enhancements
//!             2   Measure and print out volume mount time for the next scenarios:
//!                   a.  simple mount
//!                   b.  mount and read access to the media (reading the last file in the root dir.)
//!                   c.  mount and write access to the media (writing data into the last file in the root dir)
//!                   d.  mount getting volume information
//!
//!
//!             3     Turn ON using FSInfo.
//!             4     Repeat step 2 for this case.
//! @SYMTestExpectedResults Finishes ok.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void TestFAT_Mounting_Performance()
{
    test.Next(_L("\n#--> Measuring FAT volumes mount performance.\n"));
#ifndef _DEBUG
    test.Printf(_L("Skipping the test in the Release build! \n"));
    return;
#else

   //-- 1. turn OFF all mount enhancements like using FSInfo, backround FAT scan etc.
   test.Printf(_L("#--> ==== All mount enhancements are disabled ====\n"));
   SetFsyDebugFlag(gDriveNum, KMntProp_DisableALL);
   MeasureMountTime();

   //-- 2. Turn ON using FSInfo
   test.Printf(_L("#--> ==== Enabled Using FSInfo ====\n"));
   SetFsyDebugFlag(gDriveNum, KMntProp_DisableALL & ~KMntProp_Disable_FsInfo);
   MeasureMountTime();

   //-- 2. Turn OFF using FSInfo and ON FAT32 bacground scanning
   test.Printf(_L("#--> ==== Enabled FAT32 BkGnd scan, FSInfo disabled ====\n"));
   SetFsyDebugFlag(gDriveNum, KMntProp_DisableALL & ~KMntProp_Disable_FatBkGndScan);
   MeasureMountTime();

   //-- restore mounting mechanism
   SetFsyDebugFlag(gDriveNum, KMntProp_EnableALL);

#endif //_DEBUG
}



//-------------------------------------------------------------------

/**
    Check if the drive aDriveNo is finalised or not.
    The "CleanShutDown" is obtained by QueryVolumeInfoExt API and by reading 2 FATs directly with checking their consistence.

    @param  aDriveNo drive number to query.
    @return ETrue if the drive if finalised
*/
static TBool DoCheckVolumeFinalised(TInt aDriveNo)
{
    TInt nRes;
    TPckgBuf<TBool> boolPckg;

    //-- 1. get "Finalised" state by using the API
    nRes = TheFs.QueryVolumeInfoExt(aDriveNo, EIsDriveFinalised, boolPckg);
    test_KErrNone(nRes);

    //-- N.B. for FAT12 the result can be either OK or "NotSupported"
    //-- If FAT12 is in explicit "finalised" state, the result will be OK
    //-- if not, we can't query the volume state, because FAT12 doesn't support flags in FAT[1]

    const TBool bFinalised_From_API = boolPckg() >0;
          TBool bFinalised_From_FAT1 = bFinalised_From_API;

    TBuf8<32> fatBuf(32);
    TFatBootSector  bootSec;
    const TUint32 posMainBootSec = KBootSectorNum << KDefaultSectorLog2;

    nRes = ReadBootSector(TheFs, gDriveNum, posMainBootSec, bootSec);
    test_KErrNone(nRes);

    test(bootSec.IsValid());

    const TUint32 Fat1StartPos = bootSec.FirstFatSector() * bootSec.BytesPerSector();

    if(bootSec.FatType() == EFat16)
    {//-- FAT16
        TUint16 fatEntry;
        const TUint16 KClnShtdnMask = 0x8000; //-- "ClnShutBitMask", see FAT specs

        //-- read "CleanShutDown" flag directly from the 1st FAT
        nRes = MediaRawRead(TheFs, gDriveNum, Fat1StartPos, fatBuf.Size(), fatBuf);
        test_KErrNone(nRes);

        Mem::Copy(&fatEntry, (fatBuf.Ptr()+sizeof(fatEntry)), sizeof(fatEntry));
        bFinalised_From_FAT1 = (fatEntry & KClnShtdnMask) >0;


        for(TInt i=1; i<bootSec.NumberOfFats(); ++i)
        {
            //-- read a flag from the next FAT
            const TUint32 currFatStartPos = (bootSec.FirstFatSector() + i*bootSec.TotalFatSectors())*bootSec.BytesPerSector();

            nRes = MediaRawRead(TheFs, gDriveNum, currFatStartPos, fatBuf.Size(), fatBuf);
            test_KErrNone(nRes);

            Mem::Copy(&fatEntry, (fatBuf.Ptr()+sizeof(fatEntry)), sizeof(fatEntry));
            const TBool bFinalised_From_currFAT = (fatEntry & KClnShtdnMask)>0;

            test(bFinalised_From_currFAT == bFinalised_From_FAT1);
        }

    }
    else if(bootSec.FatType() == EFat32)
    {//-- FAT32
        TUint32 fatEntry;
        const TUint32 KClnShtdnMask = 0x08000000; //-- "ClnShutBitMask", see FAT specs

        //-- read "CleanShutDown" flag directly from the 1st FAT
        nRes = MediaRawRead(TheFs, gDriveNum, Fat1StartPos, fatBuf.Size(), fatBuf);
        test_KErrNone(nRes);

        Mem::Copy(&fatEntry, (fatBuf.Ptr()+sizeof(fatEntry)), sizeof(fatEntry));
        bFinalised_From_FAT1 = (fatEntry & KClnShtdnMask) >0;

        for(TInt i=1; i<bootSec.NumberOfFats(); ++i)
        {
            //-- read a flag from the next FAT
            const TUint32 currFatStartPos = (bootSec.FirstFatSector() + i*bootSec.TotalFatSectors())*bootSec.BytesPerSector();

            nRes = MediaRawRead(TheFs, gDriveNum, currFatStartPos, fatBuf.Size(), fatBuf);
            test_KErrNone(nRes);

            Mem::Copy(&fatEntry, (fatBuf.Ptr()+sizeof(fatEntry)), sizeof(fatEntry));
            const TBool bFinalised_From_currFAT = (fatEntry & KClnShtdnMask) >0;

            test(bFinalised_From_currFAT == bFinalised_From_FAT1);
        }

    }
    else  //-- FAT12
    {//-- FAT12 doesn't have flags in FAT[1]
     bFinalised_From_FAT1 = bFinalised_From_API;
    }

    test(bFinalised_From_FAT1 == bFinalised_From_API);

    return bFinalised_From_API;
}



//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_MOUNT-0522
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    RFs::FinaliseDrive() and RFs::FinaliseDrives() API
//!
//! @SYMTestActions
//!             0   Finalise the drive in RW mode and check the result.
//!             1   Finalise the drive in RW mode once again and check the result.
//!             2   Create a file and check that the volume has become unfinalised.
//!             3   Open a file, try to finalise the volume; it shall fail with KErrInUse
//!             4   close the file, finalise, check that the result is OK.
//!             5   "Unfinalise" the volume; check that the volume is not finalised any longer.
//!             6   Finalise the drive in RO mode and check the result.
//!             7   Try to create a file, shall fail with KErrAccessDenied
//!             8   Try to finalise into RW mode, shall fail with KErrAccessDenied
//!             9   Try to unfinalise volume, it shall remain RO
//!             10  Remount the drive, it shall become RW and finalised
//!             11  Test transition "Not finalised" -> EFinal_RW (expected: KErrNone)
//!             12  Test transition EFinal_RW -> EFinal_RO  (expected: KErrNone)
//!             13  Test transition EFinal_RO -> EFinal_RW  (expected: KErrAccessDenied)
//!             14  Remount the volume to reset RO flag
//!             15  test old RFs::FinaliseDrives() API by finalising all drives in the system
//!
//! @SYMTestExpectedResults finishes if the volume finalisation works correctly. panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
static void TestFinaliseFS()
{
    test.Next(_L("Testing RFs::FinaliseDrives() API\n"));

    if(!Is_Fat16(TheFs, gDriveNum) && !Is_Fat32(TheFs, gDriveNum))
    {
        test.Printf(_L("This step requires FAT16 or FAT32 ! Skipping.\n"));
        return;
    }

    TInt  nRes;
    TBool bDriveFinalised;

    //============= 1. finalise the drive (RW mode) and check the result
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test_KErrNone(nRes);

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    //-- 1.1 finalise the drive second time EFinal_RW -> EFinal_RW shall work
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test_KErrNone(nRes);

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    //============= 2. create a file. Shall succeed (EFinal_RW), the volume shall become unfinalised

    RFile file;
    _LIT(KFileName, "\\my_file1.dat");

    nRes = CreateEmptyFile(TheFs, KFileName, 128000);
    test_KErrNone(nRes);

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(!bDriveFinalised); //-- the volume has become "unfinalised"

    //-- 2.1 open a file, try to finalise; This should be OK
    nRes = file.Replace(TheFs, KFileName, EFileWrite | EFileRead);
    test_KErrNone(nRes);

    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test_KErrNone(nRes);

    file.Close();

    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test_KErrNone(nRes);

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    //============= 3. test "unfinalise API"
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EForceUnfinalise);
    test_KErrNone(nRes);

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(!bDriveFinalised); //-- the volume has become "unfinalised"

    //============= 4. test finalisation into RO mode
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RO); //-- the volume becomes RO
    test_KErrNone(nRes);

    //-- try to write a file on RO volume; it shall fail with KErrAccessDenied
    nRes = CreateEmptyFile(TheFs, KFileName, 128000);
    test(nRes == KErrAccessDenied);
    file.Close();

    //-- 4.1 try to finalise into EFinal_RW mode, shall fail with KErrAccessDenied
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test(nRes == KErrAccessDenied);

    //-- 4.2 "unfinalise" the volume, it still shall remain RO
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EForceUnfinalise);
    test_KErrNone(nRes);

    //-- try to write a file on RO volume; it shall fail with KErrAccessDenied
    nRes = CreateEmptyFile(TheFs, KFileName, 128000);
    test(nRes == KErrAccessDenied);
    file.Close();

    //-- remount FS, the drive shall become RW
    nRes = RemountFS(TheFs, gDriveNum);
    test_KErrNone(nRes);

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    //-- try to write a file on RW volume, shall be OK
    nRes = CreateEmptyFile(TheFs, KFileName, 128000);
    test(nRes == KErrNone);
    file.Close();

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(!bDriveFinalised);

    //============= 5. test various finalisation modes

    //-- 5.1  Not finalised -> EFinal_RW (KErrNone)
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test(nRes == KErrNone);
    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    //-- 5.2  EFinal_RW -> EFinal_RO (KErrNone)
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RO);
    test(nRes == KErrNone);
    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    //-- 5.2  EFinal_RO -> EFinal_RW  (KErrAccessDenied)
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test(nRes == KErrAccessDenied);
    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    //-- 5.3 restore
    nRes = RemountFS(TheFs, gDriveNum);
    test_KErrNone(nRes);


    //============= 6. test old RFs::FinaliseDrives API

    nRes = CreateEmptyFile(TheFs, KFileName, 128000);
    test(nRes == KErrNone);

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(!bDriveFinalised);

    TheFs.FinaliseDrives(); //-- shall work as TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW) but for ALL drives

    bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    nRes = CreateEmptyFile(TheFs, KFileName, 128000);
    test(nRes == KErrNone);

}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_MOUNT-0523
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    testing boot and backup boot sectors on FAT32 volume
//!
//! @SYMTestActions
//!             0   Quick format the drive
//!             1   read main and backup boot & fsinfo sectors and check their validity
//!             2   corrupt main boot sector and check that the drive can be mounted (backup boot sector is used)
//!             3   corrupt backup boot sector and check that the drive can not be mounted.
//!             4   Quick format the drive to restore test environment
//!
//! @SYMTestExpectedResults finishes if the boot sector and backup boot sector functionality compliant with the FAT specs. panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
static void TestBackupBootSector()
{
    test.Next(_L("Testing Backup Boot Sector.\n"));

    TInt nRes;

    //-- quick format the drive
    FormatVolume(ETrue);

    if(!Is_Fat32(TheFs, gDriveNum))
    {
        test.Printf(_L("This step requires FAT32 ! Skipping.\n"));
        return;
    }

    TFatBootSector  mainBootSec, backupBootSec;
    TFSInfo         mainFSInfo, backupFSInfo;

    const TUint32 posMainBootSec = KBootSectorNum << KDefaultSectorLog2;
    const TUint32 posBkBootSec   = KBkBootSectorNum << KDefaultSectorLog2;

    //-- read main and backup boot & fsinfo sectors and check their validity
    nRes = ReadBootSector(TheFs, gDriveNum, posMainBootSec, mainBootSec);
    test_KErrNone(nRes);

    //-- backup boot sector # must be 6
    nRes = ReadBootSector(TheFs, gDriveNum, posBkBootSec, backupBootSec);
    test_KErrNone(nRes);

    test(mainBootSec.IsValid());
    test(backupBootSec.IsValid());
    test(mainBootSec == backupBootSec);

    //-- read fsinfo sectors
    const TUint32 posMainFSInfo = mainBootSec.FSInfoSectorNum() << KDefaultSectorLog2;
    const TUint32 posBkFSInfo   = (KBkBootSectorNum + mainBootSec.FSInfoSectorNum()) << KDefaultSectorLog2;

    test(posMainFSInfo != 0);
    test(posBkFSInfo != 0);

    nRes = ReadFSInfoSector(TheFs, gDriveNum, posMainFSInfo, mainFSInfo);
    test_KErrNone(nRes);

    nRes = ReadFSInfoSector(TheFs, gDriveNum, posBkFSInfo, backupFSInfo);
    test_KErrNone(nRes);

    test(mainFSInfo.IsValid());
    test(backupFSInfo.IsValid());
    test(mainFSInfo == backupFSInfo);

    //-- corrupt main boot sector and check that the drive can be mounted
    test.Printf(_L("Corrupting main boot sector...\n"));


    //-- A1. corrupt main boot sector starting from the pos:0
    nRes = FillMedia(TheFs, gDriveNum, posMainBootSec, posMainBootSec+KDefaultSectorSize, 0xaa);

    //-- A2. remount FS, it shall be OK because of the using backup boot sector
    nRes = RemountFS(TheFs, gDriveNum);
    test_KErrNone(nRes);


    //-- B1. corrupt BACKUP boot sector starting from the sec:6
    nRes = FillMedia(TheFs, gDriveNum, posBkBootSec, posBkBootSec+KDefaultSectorSize, 0xbb);

    //-- B2. remount FS, unable to mount.
    nRes = RemountFS(TheFs, gDriveNum);
    test(nRes == KErrCorrupt);


    //-- quick format the drive
    FormatVolume(ETrue);

}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_MOUNT-0524
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1721
//! @SYMTestCaseDesc    testing FSInfo sector functionality.
//!
//! @SYMTestActions
//!             0   Quick format the drive
//!             1   finalise the drive to write correct data to the FSInfo & its backup copy.
//!             2   read FSInfo sector, its backup copy and check that they are both valid, identical and contain correct data
//!             3   check that after the formatting FS info values are the same as real, obtained from the FAT scanning.
//!             4   create a random - sized file, check that after finalisation the number of free clusters is identical to the FSinfo
//!             5   Quick format the drive to restore test environment
//!
//! @SYMTestExpectedResults finishes if the FSInfo sectors functionality compliant with the FAT specs. panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
static void TestFSInfoSector()
{
    test.Next(_L("Testing FSInfo Sector.\n"));

#ifndef _DEBUG
    test.Printf(_L("Skipping the test in the Release build! \n"));
    return;
#else

    TInt nRes;

    //-- quick format the drive
    FormatVolume(ETrue);

    if(!Is_Fat32(TheFs, gDriveNum))
    {
        test.Printf(_L("This step requires FAT32 ! Skipping.\n"));
        return;
    }

    TFatBootSector  bootSec;
    const TUint32 posMainBootSec = KBootSectorNum << KDefaultSectorLog2;
    nRes = ReadBootSector(TheFs, gDriveNum, posMainBootSec, bootSec);
    test_KErrNone(nRes);

    const TUint32 bytesPerSector = bootSec.BytesPerSector();
    const TUint32 secPerClust = bootSec.SectorsPerCluster();

    //-- finalise the drive, just in case
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test_KErrNone(nRes);

    //============= 1. read FSInfo sector and its backup copy and compare them
    TFSInfo fsInfoSec;

    //-- main FSInfo
    nRes = ReadFSInfoSector(TheFs, gDriveNum, KFSInfoSectorNum*bytesPerSector, fsInfoSec);
    test_KErrNone(nRes);
    test(fsInfoSec.IsValid());

    TUint32 freeClusters_FSInfo = fsInfoSec.FreeClusterCount();
    TUint32 nextFree_FSInfo = fsInfoSec.NextFreeCluster();

    //-- backup FSInfo
    nRes = ReadFSInfoSector(TheFs, gDriveNum, KBkFSInfoSectorNum*bytesPerSector, fsInfoSec);
    test_KErrNone(nRes);
    test(fsInfoSec.IsValid());

    //-- both copies must be identical
    test(freeClusters_FSInfo == fsInfoSec.FreeClusterCount());
    test(nextFree_FSInfo == fsInfoSec.NextFreeCluster());

    //-- FAT[0] and FAT[1] are not used; FAT[2] is taken by the 1st cluster of the FAT32 Root directory.
    test(nextFree_FSInfo == (2+1));

    //============= 2. check that after the formatting FS info values are the same as real.

    //-- 2.1 disable using FSInfo and other stuff
    SetFsyDebugFlag(gDriveNum, KMntProp_Disable_FsInfo);

    //-- remount FAT; using FSInfo is disabled. FAT will be explicitly scanned.
    nRes = RemountFS(TheFs, gDriveNum);
    test_KErrNone(nRes);

    //-- restore mounting mechanism
    SetFsyDebugFlag(gDriveNum, KMntProp_EnableALL);

    //-- get free clusters number from the FSY, which in turn had counted them explicitly
    TVolumeInfo volInfo;
    nRes = TheFs.Volume(volInfo, gDriveNum);
    test_KErrNone(nRes);

    TUint32 freeClusters = (TUint32)(volInfo.iFree / (bytesPerSector * secPerClust));
    test(freeClusters == freeClusters_FSInfo);

    //============= 3. create a random - sized file, check that after finalisation the number of free clusters is identical to the FSinfo
    _LIT(KFileName, "\\FILE1.DAT");
    const TUint32 rndClusters = 7+((TUint32)Math::Rand(gRndSeed)) % 5000;
    const TUint32 fileSz = rndClusters * bytesPerSector * secPerClust;
    nRes = CreateEmptyFile(TheFs, KFileName, fileSz);
    test_KErrNone(nRes);

    //-- 3.1 get data from FS
    nRes = TheFs.Volume(volInfo, gDriveNum);
    test_KErrNone(nRes);
    freeClusters = (TUint32)(volInfo.iFree / (bytesPerSector * secPerClust));

    //-- 3.2 finalise the volume and get data from FSInfo
    nRes =TheFs.FinaliseDrive(gDriveNum, RFs::EFinal_RW);
    test_KErrNone(nRes);

    //-- main FSInfo
    nRes = ReadFSInfoSector(TheFs, gDriveNum, KFSInfoSectorNum*bytesPerSector, fsInfoSec);
    test_KErrNone(nRes);
    test(fsInfoSec.IsValid());

    freeClusters_FSInfo = fsInfoSec.FreeClusterCount();
    nextFree_FSInfo = fsInfoSec.NextFreeCluster();

    //-- backup FSInfo
    nRes = ReadFSInfoSector(TheFs, gDriveNum, KBkFSInfoSectorNum*bytesPerSector, fsInfoSec);
    test_KErrNone(nRes);
    test(fsInfoSec.IsValid());

    //-- both copies must be identical
    test(freeClusters_FSInfo == fsInfoSec.FreeClusterCount());
    test(nextFree_FSInfo == fsInfoSec.NextFreeCluster());

    //-- the information in FSInfo must be the same as in FAT
    test(freeClusters == freeClusters_FSInfo);

    TBool bDriveFinalised = DoCheckVolumeFinalised(gDriveNum);
    test(bDriveFinalised);

    TheFs.Delete(KFileName);

    //-- restore mounting mechanism
    SetFsyDebugFlag(gDriveNum, KMntProp_EnableALL);

#endif //_DEBUG
}

//-------------------------------------------------------------------

/** initialise test global objects */
static void InitGlobals()
{
    TInt nRes;

    //-- define a propery which will control mount process in the fsy.
    //-- The property key is a drive number being tested

    _LIT_SECURITY_POLICY_PASS(KTestPropPolicy);
    nRes = RProperty::Define(KThisTestSID, gDriveNum, RProperty::EInt, KTestPropPolicy, KTestPropPolicy);
    test(nRes == KErrNone || nRes == KErrAlreadyExists);

    nRes = RProperty::Set(KThisTestSID, gDriveNum, KMntProp_EnableALL);
    test_KErrNone(nRes);

    gRndSeed = Math::Random();
    (void)&gRndSeed; //-- get rid of warning
}

/** destroy test global objects */
static void DestroyGlobals()
{
    //-- delete test property
    RProperty::Delete(KThisTestSID, gDriveNum);

    TVolumeInfo v;
    TheFs.Volume(v);
}

//-------------------------------------------------------------------
/**
    Manual test. Requires manual removing and putting back the media.
    On the emulator one can use pressing (and holding) F5 key to simulate media removal.
*/
void Manual_TestRemount_On_MediaRemoval()
{
    TInt nRes;

    _LIT(KFileName, "\\my_file1.dat");
    const TUint32 KFileSz = K1MegaByte;

    //-- 1. create a file
    nRes = CreateEmptyFile(TheFs, KFileName, KFileSz);
    test_KErrNone(nRes);

    RFile file;

    nRes = file.Open(TheFs, KFileName, EFileRead | EFileWrite);
    test_KErrNone(nRes);

    TBuf8<512> buf(512);
    TVolumeInfo vi;
    buf.FillZ();

    TKeyCode key;

    for(;;)
    {
        TheFs.SetDebugRegister(0x00);

        nRes = file.Read(0, buf);

        test.Printf(_L("Remove the media and press a key.\n"));
        key = test.Getch();
        if(key == EKeyEscape)
                break;

        TheFs.SetDebugRegister(KFSYS | KFSERV);
        nRes = file.Read(0, buf);

        if(nRes != KErrNone)
        {

            test.Printf(_L("ReadFile: %d!\n"), nRes);

            key = test.Getch();
            if(key == EKeyEscape)
                break;

            nRes = TheFs.Volume(vi,gDriveNum);
            test.Printf(_L("Volume: %d!\n"), nRes);

            key = test.Getch();
            if(key == EKeyEscape)
                break;

            nRes = file.Write(0, buf);
            test.Printf(_L("WriteFile: %d!\n"), nRes);

            key = test.Getch();
            if(key == EKeyEscape)
                break;

        }

    }


    file.Close();
}


//-------------------------------------------------------------------
/**
    Wait for the request aRqStat to be completed with timeout.

    @param  aRqStat         request status object we need to wait to complete
    @param  aTimeout_uS     timeout in microseconds

    @return ETrue   if the aRqStat is completed before time is out
            EFalse  if aTimeout_uS has passed. And the state of the aRqStat not changed.
*/
TBool WaitForRequestWithTimeout(TRequestStatus& aRqStat, TUint32 aTimeout_uS)
{
    TRequestStatus  rqStatTimeout(KRequestPending);
    RTimer          tmrTimeOut;
    TInt            nRes;
    TBool           bReqCompleted;

    if(aRqStat.Int() != KRequestPending)
        return ETrue; //-- nothing to wait for.

    //-- set up a timeout timer
    nRes = tmrTimeOut.CreateLocal();
    test(nRes == KErrNone);

    tmrTimeOut.After(rqStatTimeout, aTimeout_uS);

    User::WaitForRequest(aRqStat, rqStatTimeout);

    if(aRqStat == KRequestPending)
        {//-- timeout.
        bReqCompleted = EFalse;
        }
        else
        {//-- the main request has been completed, cancel timer
            bReqCompleted = ETrue;
            if(rqStatTimeout.Int() == KRequestPending)
                {
                tmrTimeOut.Cancel();
                User::WaitForRequest(rqStatTimeout);
                }
        }

    tmrTimeOut.Close();

    return bReqCompleted;
}

//-------------------------------------------------------------------

void TestNotifyDiskSpace()
{
    test.Next(_L("Testing NotifyDiskSpace() on asynchronous mounting\n"));

#ifndef _DEBUG
    test.Printf(_L("Skipping the test in the Release build! \n"));
    return;
#else

    TInt nRes;
    TRequestStatus rqStat;
    TVolumeInfo volInfo;

    //-- quick format the drive
    FormatVolume(ETrue);

    if(!Is_Fat32(TheFs, gDriveNum))
    {//-- only FAT32 supports asynch mounting;
        test.Printf(_L("This test step requires FAT32!\n"));
        return;
    }

    //-- FAT32 free space threshold that is supposed to be triggered by notifiers
    const TInt64 KFreeSpaceThreshold = 300*K1MegaByte;

    //-- Turn OFF using FSInfo and ON FAT32 background scanning; it will cause compulsory FAT32 background free clusters scan.
    //-- if FAT32 background free clusters scan is disabled in config, this test can hang on waiting for free space notifications.
    test.Printf(_L("==== Enabled FAT32 BkGnd scan, FSInfo disabled ====\n"));
    SetFsyDebugFlag(gDriveNum, KMntProp_DisableALL & ~KMntProp_Disable_FatBkGndScan);


    //===== create a big file in the beginning in order to avoid freeSpaceThreshold being reached too fast
    nRes = CreateEmptyFile(TheFs, _L("\\big_empty_file.bin"), 10*K1MegaByte);
    test_KErrNone(nRes);

//TheFs.SetDebugRegister(0x03);

    //===== Test that FAT32 free space scanning thread updates File Server free space notifiers
    test.Printf(_L("Testing FAT32 free space scanning thread triggers notifiers..."));

    nRes = RemountFS(TheFs, gDriveNum); //-- remount FS; it must cause FAT32 free space scan in background
    test_KErrNone(nRes);

    test.Printf(_L("Waiting for %LU bytes available on the volume...\n"), KFreeSpaceThreshold);

    //-- check if we can use the notifiers at all... If free space is >= KFreeSpaceThreshold, the notifier won't be triggered
    //-- get _current_ amount of free space asynchronously
    TheFs.Volume(volInfo, gDriveNum, rqStat);
    User::WaitForRequest(rqStat);
    test(rqStat.Int()==KErrNone);

    if(volInfo.iSize <= KFreeSpaceThreshold)
    {
    test.Printf(_L("The volume is too small for %LU bytes notify ...\n"), KFreeSpaceThreshold);
    test.Printf(_L("The test is inconclusive ...\n"));
    return;
    }

    if(volInfo.iFree >= KFreeSpaceThreshold)
    {
    test.Printf(_L("The volume already has %LU free bytes...\n"), volInfo.iFree);
    test.Printf(_L("The test is inconclusive ...\n"));
    return;
    }


    TheFs.NotifyDiskSpace(KFreeSpaceThreshold, gDriveNum, rqStat);
    test(rqStat.Int()==KRequestPending);

    //-- wait for notification for 30 seconds; If for some reason FAT32 background scanning for free clusters doesn't
    //-- work (e.g. configued out in FAT), this test is inconclusive.
    TBool bCompleted = WaitForRequestWithTimeout(rqStat, 30*K1Sec);

    if(!bCompleted)
    {
        test.Printf(_L("Wait timeout! something is wrong...\n"));
        test(0);
    }

    test_KErrNone(rqStat.Int());

    //-- get _current_ amount of free space asynchronously
    TheFs.Volume(volInfo, gDriveNum, rqStat);
    User::WaitForRequest(rqStat);
    test(rqStat.Int()==KErrNone);
    test.Printf(_L("Current amount of free space on the volume: %LU \n"), volInfo.iFree);


    //===== Test that aborting FAT32 free space scanning thread will trigger notifiers
    test.Printf(_L("Testing aborting FAT32 free space scanning thread..."));

    nRes = RemountFS(TheFs, gDriveNum); //-- remount FS; it must cause FAT32 free space scan in background
    test_KErrNone(nRes);

    TheFs.NotifyDiskSpace(KFreeSpaceThreshold, gDriveNum, rqStat);
    test(rqStat.Int()==KRequestPending);

    nRes = RemountFS(TheFs, gDriveNum); //-- remount FS; it will abort the scanning thread
    test_KErrNone(nRes);

    test(rqStat.Int() != KRequestPending);

    //-- get _current_ amount of free space asynchronously
    TheFs.Volume(volInfo, gDriveNum, rqStat);
    User::WaitForRequest(rqStat);
    test(rqStat.Int()==KErrNone);
    test.Printf(_L("Current amount of free space on the volume: %LU \n"), volInfo.iFree);


    //-- find out free space on the volume; it will also blocks until FAT32 free space scanning finishes.
    nRes = TheFs.Volume(volInfo);
    test(nRes==KErrNone);
    test.Printf(_L("free space on the volume: %LU \n"), volInfo.iFree);


//TheFs.SetDebugRegister(0x00);

    //-- restore mounting mechanism
    SetFsyDebugFlag(gDriveNum, KMntProp_EnableALL);

#endif  //_DEBUG
}

//-------------------------------------------------------------------

void CallTestsL()
    {
    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());

    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test(nRes==KErrNone);

    //-- check if this is FAT
    if(!Is_Fat(TheFs, gDriveNum) || Is_Automounter(TheFs, gDriveNum))
    {//-- it doesn't make much sense to run this test under automounter+FAT. The automounter can't easily handle formatting corrupted media
     //-- and the mounting permaormance measurements don't make much sense in this case as well.

        test.Printf(_L("Skipping. This test requires explicitly mounted FAT file system.\n"));
        return;
    }

    //-- check this is not the internal ram drive
    TVolumeInfo v;

    nRes = TheFs.Volume(v);
    test(nRes==KErrNone);
    if(v.iDrive.iMediaAtt & KMediaAttVariableSize)
        {
        test.Printf(_L("Skipping. Internal ram drive not tested.\n"));
        return;
        }

    //-------------------------------------

    PrintDrvInfo(TheFs, gDriveNum);
    InitGlobals();

    TestNotifyDiskSpace();

    //-------------------------------------
    TestBackupBootSector();
    TestFSInfoSector();
    TestFinaliseFS();

    //-------------------------------------
    TestFAT_Mounting_Performance();
    //-------------------------------------
    //-- manual test
    //Manual_TestRemount_On_MediaRemoval();

    //-------------------------------------
    DestroyGlobals();

    }


















