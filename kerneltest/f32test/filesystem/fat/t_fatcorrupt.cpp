// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_fatcorrupt.cpp
//
//

//! @SYMTestCaseID FSBASE-CR-JHAS-68YPX7
//! @SYMTestType CT
//! @SYMREQ CR JHAS-68YPX7
//! @SYMTestCaseDesc Test functionality of handling corrupt data on disk
//! @SYMTestStatus Implemented
//! @SYMTestActions Use fat test extension to test read/write/format
//! @SYMTestExpectedResults Read bad sector fail, others pass
//! @SYMTestPriority Medium
//! @SYMAuthor Ying Shi
//! @SYMCreationDate 20/05/2005
//! @See EFat and EFat32 components
//! @file f32test\server\t_fatcorrupt.cpp

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32math.h>

#include "t_server.h"
#include "t_fatext.h"

using namespace Fat_Test_Utils;

GLDEF_D RTest test(_L("T_FATCORRUPT"));

static TInt gDrive;
static TBuf16<2> gDriveName(2);
static TFileName gFileSystem;
static TFullName gExtName;
static TBool gExtExists=EFalse;
static TFatBootSector BootSector;

static TInt gBadList[3];

_LIT(KExtName,"FATTEST");


void RestoreEnv();

/**
    Do format steps and print progress

    @param  aFormat     specifies the format object
    @param  aStepsCnt   format steps counter from RFormat::Open

    @return format completion error code
*/
static TInt DoFormatSteps(RFormat& aFormat, TInt aStepsCnt)
{
    TInt nRes = KErrNone;
    TInt prevCnt = aStepsCnt;

    while (aStepsCnt && nRes == KErrNone)
        {
         if(prevCnt != aStepsCnt)
            {
            test.Printf(_L("."));
            prevCnt = aStepsCnt;
            }

        nRes = aFormat.Next(aStepsCnt);
        }

    test.Printf(_L("\n\r"));

    return nRes;
}


#define TESTL(con) TestL(con,r,__LINE__)

void TestL(TBool aCondition, TInt aError, TInt aLine = 0)
    {
    if (!aCondition)
        {
        RDebug::Printf("Test fail at line: %d with error code %d",aLine, aError);
        RestoreEnv();
        }
    test(aCondition);
    }

//-------------------------------------------------------------

void RestoreEnv()
    {
    test.Printf(_L("RestoreEnv()\n"));

    TInt r = TheFs.DismountExtension(KExtName, gDrive);
    test_KErrNone(r);
    r = TheFs.RemoveExtension(KExtName);
    test_KErrNone(r);

    if(gExtExists)      // remount existing secondary extension
        {
        test(TheFs.MountExtension(gExtName, gDrive)==KErrNone);
        }


    //-- We need a full format, because quick format preserves bad sectors marked in FAT
    r = FormatFatDrive(TheFs, gDrive, EFalse);
    test_KErrNone(r);

    }

//-------------------------------------------------------------

// Remount drive with FAT test extension
TBool PrepareMount()
    {
    test.Printf(_L("PrepareMountL(), drive:%d \n"),gDrive);

    TInt r = TheFs.AddExtension(KExtName);
    test_Value(r, r == KErrNone || r==KErrAlreadyExists);
    r = TheFs.ExtensionName(gExtName,gDrive,0);
    if (r == KErrNone)              // an extension already exists -> dismount it
        {
        test.Printf(_L("Drive %d has extension, attempt dismounting it\n"),gDrive);
        r=TheFs.DismountExtension(gExtName,gDrive);
        if(r==KErrAccessDenied)     // primary extension
            {
            test.Printf(_L("Drive %d has primary extension, skip\n"),gDrive);
            test(TheFs.RemoveExtension(KExtName) == KErrNone);
            return EFalse;
            }
        test_KErrNone(r);
        gExtExists=ETrue;
        }
    test_Value(r, r == KErrNone || r==KErrNotFound);
    r = TheFs.MountExtension(KExtName, gDrive);
    if (r != KErrNone)
        test(TheFs.RemoveExtension(KExtName) == KErrNone);
    test_KErrNone(r);
    return ETrue;
    }

//-------------------------------------------------------------

void MarkClustersL()
    {
    TInt size = sizeof(gBadList) / sizeof(TInt);
    for (TInt i=0; i<size; i++)
        {
        TInt r = TheFs.ControlIo(gDrive,CFatTestProxyDrive::EMark,(TAny*)gBadList[i],NULL);
        TESTL(r == KErrNone);
        }
    }

//-------------------------------------------------------------

/**
    Create 3 empty files on the volume
*/
void CreateFiles()
    {
    TInt r;
    RFile file;
    TBuf<10> fileName01;
    fileName01.Append(_L("?:\\file01"));
    fileName01[0] = (TUint16)gDriveToTest;
    r = file.Create(TheFs, fileName01, EFileWrite);
    TESTL(r==KErrNone||r==KErrAlreadyExists);
    file.Close();

    TBuf<10> fileName02;
    fileName02.Append(_L("?:\\file02"));
    fileName02[0] = (TUint16)gDriveToTest;
    r = file.Create(TheFs, fileName02, EFileWrite);
    TESTL(r==KErrNone||r==KErrAlreadyExists);
    file.Close();

    TBuf<10> fileName03;
    fileName03.Append(_L("?:\\file03"));
    fileName03[0] = (TUint16)gDriveToTest;
    r = file.Create(TheFs, fileName03, EFileWrite);
    TESTL(r==KErrNone||r==KErrAlreadyExists);
    file.Close();
    }

//-------------------------------------------------------------

/**
    Read/Write to the file with damaged clusters
*/
void DoTestReadWriteL()
    {
    CreateFiles();

    // Get disk mount information
    TPckgBuf<TInt> dataPositionBuf;
    TPckgBuf<TInt> sectorsPerClusterBuf;

    TInt r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EGetDataPosition,dataPositionBuf);
    TESTL(r==KErrNone);

    //-- media position of the 1st data sector on the volume.
    //-- for FAT32 this is the 2nd cluster of the data ares, because 1st belongs to the root dir.
    const TInt dataPosition = dataPositionBuf();

    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::ESectorsPerCluster,sectorsPerClusterBuf);
    TESTL(r==KErrNone);

    const TInt sectorsPerCluster = sectorsPerClusterBuf(); //-- FAT sectors per cluster



    // Calculate file data sectors
    // For FAT32, first data sector falls under root directory, and current FAT32
    // file system implementation does not take care of handling bad sectors in
    // root directory so changing start sector to be the start sector of the file,
    // instead of first data sector.
    TInt f01_StartSector;
    if(Is_Fat32(TheFs,gDrive))
    {
        f01_StartSector = (dataPosition >> KDefaultSectorLog2);
        test.Printf(_L("FAT32, dataPosition: %d, sec:%d, spc:%d\n"), dataPosition, f01_StartSector, sectorsPerCluster);
    }
    else
    {
        f01_StartSector = (dataPosition >> KDefaultSectorLog2);
        test.Printf(_L("dataPosition: %d, sec:%d, spc:%d\n"), dataPosition, f01_StartSector, sectorsPerCluster);
    }

    //TInt f01end = f01beg + sectorsPerCluster - 1;
    //TInt f02beg = f01end + 1;
    //TInt f02end = f02beg + sectorsPerCluster - 1;
    //TInt f03beg = f02end + 1;
    //TInt f03end = f03beg + sectorsPerCluster - 1;

    test.Next(_L("Test file write"));

    //-- mark the sector that file01 starts with as bad
    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EMark,(TAny*)(f01_StartSector),NULL);
    TESTL(r==KErrNone);

    TBuf<10> fileName01;
    fileName01.Append(_L("?:\\file01"));
    fileName01[0] = (TUint16)gDriveToTest;

    RFile file;

    //-- write to the file with bad sector
    r = file.Open(TheFs, fileName01, EFileWrite);
    TESTL(r==KErrNone);

    TInt size = 2 * sectorsPerCluster * KDefaultSectorSize;   //-- 2 clusters
    RBuf8 writeBuf01;

    r = writeBuf01.CreateMax(size);
    TESTL(r == KErrNone);

    r = file.Write(writeBuf01, size);
    TESTL(r==KErrNone);

    file.Close();

    test.Next(_L("Test file read"));

    //-- unmark bas sector
    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EUnmark,(TAny*)(f01_StartSector),NULL);
    TESTL(r==KErrNone);

    //-- mark the 2nd cluster in the file as bad
    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EMark,(TAny*)(f01_StartSector+sectorsPerCluster),NULL);
    TESTL(r==KErrNone);

    r = file.Open(TheFs, fileName01, EFileRead);
    TESTL(r==KErrNone);

    //-- read the file from "corrupted" media
    r = file.Read(writeBuf01, size);
    TESTL(r==KErrCorrupt); //-- the file must be corrupted

    file.Close();

    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EUnmarkAll, NULL, NULL);
    TESTL(r==KErrNone);

    writeBuf01.Close();

    }


//-------------------------------------------------------------------------------------------------------

/**
    Suggest FAT volume metrics by volume size in sectors. Pay attention to the KMaxSectorsPerCluster constant.

    @param  aVolSizeInSectors   FAT volume size in sectors
    @param  apSectPerCluster    on return here will be suggested number of sectors per cluster
    @param  apRootDirEntries    on return here will be suggested max. number of root dir. entries.
*/
void SuggestVolMetrics(TUint aVolSizeInSectors, TUint16 *apSectPerCluster, TUint *apRootDirEntries=NULL)
{
    const TUint16  KMaxSectorsPerCluster = 32; //-- maximal number sectors per cluster to use
          TUint16  sectorsPerCluster;
          TUint    rootDirEntries;

    //-- suggest Sector per Cluster value, most suitable for the given volume size. (taken from CFatFormatCB::InitFormatDataForFixedSizeDiskUser(...))
    if (aVolSizeInSectors < 4096) // < 2MB
        {
        sectorsPerCluster = 1;
        rootDirEntries    = 128;
        }
    else if (aVolSizeInSectors < 8192) // < 4MB
        {
        sectorsPerCluster = Min(KMaxSectorsPerCluster, 2);
        rootDirEntries = 256;
        }
    else if (aVolSizeInSectors < 32768) // < 16MB
        {
        sectorsPerCluster = Min(KMaxSectorsPerCluster, 4);
        rootDirEntries = 512;
        }
    else if (aVolSizeInSectors < 1048576) // < 512MB
        {
        sectorsPerCluster = Min(KMaxSectorsPerCluster, 8);
        rootDirEntries = 512;
        }
    else // FAT32
        {
        sectorsPerCluster = KMaxSectorsPerCluster;
        rootDirEntries = 512;
        }

    //---------------------------------------
    if(apSectPerCluster)
        *apSectPerCluster = sectorsPerCluster;

    if(apRootDirEntries)
        *apRootDirEntries = rootDirEntries;

}

//-------------------------------------------------------------

/**
    Test user-specific FAT format. FAT type 12,16 or 32 will depend on disk size.
    @param  aQuick if not 0 a quick format will be performed.
*/
void DoTestUserFormatL(TBool aQuick, TBool aMark)
    {
    const TUint32   KKiloByte = 1024;

    TInt r;

    //-- obtain current FAT volume metrics
    TVolumeInfo volInfo;
    r = TheFs.Volume(volInfo, gDrive);
    TESTL(r == KErrNone);

    const TInt64    volSize = volInfo.iSize; //-- volume size in bytes
    const TUint     volSizeInSectors = TUint(volSize >> KDefaultSectorLog2); //-- sectors per volume

    TUint16   sectorsPerCluster=0;
    TUint     rootDirEntries = 0;
    TUint     fatType = 0;

    //-- roughly suggest FAT type for this volume, if FAT type is incorrect, RFormat will fail.
    //-- suggest Sector per Cluster value, most suitable for this volume size  (taken from CFatFormatCB::InitFormatDataForFixedSizeDiskUser(...))
    SuggestVolMetrics(volSizeInSectors, &sectorsPerCluster, &rootDirEntries);

    const TUint totVolClusterCnt = volSizeInSectors / sectorsPerCluster; //-- total clusters per this volume

    //-- doesn't count here FAT size, RootDir size and reserved sectors. For rough estimations only, otherwise
    //-- it will be far too complicated.
    const TUint dataClusterCnt = totVolClusterCnt;

    //-- magic. see FAT specs for details.
    if(dataClusterCnt < 4085)
        fatType = TLDFormatInfo::EFB12;
    else if(dataClusterCnt < 65525)
        fatType = TLDFormatInfo::EFB16;
    else
        fatType = TLDFormatInfo::EFB32;

    test.Printf(_L("Formatting drive %C:, size=%uKB, SecPerClust=%u, FAT%d\n"), ('A'+gDrive), (TUint32)(volSize/KKiloByte), sectorsPerCluster, fatType);

    //-- formatting the volume
    TInt formatCnt;
    TLDFormatInfo formatInfo;

    formatInfo.iSectorsPerCluster = sectorsPerCluster;
    formatInfo.iFATBits = (TLDFormatInfo::TFATBits)fatType;

    TSpecialFormatInfoBuf formatInfoBuf(formatInfo);
    RFormat formatUser;

    if (aQuick)
        {
        r = formatUser.Open(TheFs, gDriveName, ESpecialFormat|EQuickFormat, formatCnt, formatInfoBuf);
        TESTL(r == KErrNone);
        }
    else
        {
        r = formatUser.Open(TheFs, gDriveName, ESpecialFormat, formatCnt, formatInfoBuf);
        TESTL(r == KErrNone);
        }

    if(aMark)
        {
        MarkClustersL();
        }

    r = DoFormatSteps(formatUser, formatCnt);
    TESTL(r == KErrNone || r == KErrNotSupported);

    formatUser.Close();

    if (r == KErrNotSupported)
        {
        test.Printf(_L("Media does not support ESpecialFormat"));
        return;
        }

    //-- format has finished, Check format validity
    TPckgBuf<TInt> sbuf;
    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::ESectorsPerCluster, sbuf);
    TESTL(r==KErrNone);
    const TInt resSectorsPerCluster = sbuf();

    TPckgBuf<TInt64> sizebuf;
    r = TheFs.ControlIo(gDrive, CTestProxyDrive::EDiskSize, sizebuf);
    TESTL(r==KErrNone);
    const TInt64 resVolSize = sizebuf();

    //-- Check fat type
    TPckgBuf<TInt> tbuf;
    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EFatType, tbuf);
    TESTL(r==KErrNone);
    const TUint resFatType = tbuf();

    test.Printf(_L("Format result:, size=%uKB, SecPerClust=%u, FAT%d\n"), (TUint32)(resVolSize/KKiloByte), resSectorsPerCluster, resFatType);

    TESTL(resFatType == fatType);

    //-- user-defined format CFatFormatCB::InitFormatDataForFixedSizeDiskUser(TInt aDiskSizeInSectors)
    //-- can decide decides how many SectorsPerCluster will be on the volume. As the result, requested and resulted values can be different.
    TESTL(resSectorsPerCluster == sectorsPerCluster);

    //-- check whole drive
    test.Next(_L("Check Disk"));
    r = TheFs.CheckDisk(gDriveName);
    TESTL(r==KErrNone);
    }

//-------------------------------------------------------------

/** Randomly generate several sector numbers in FAT data area, which will be marked as bad. */
static void CreateBadList()
{
    static TInt64 rndSeed = Math::Random();
    const TInt size = sizeof(gBadList) / sizeof(TInt);

    //const TUint32 dataStartSector = BootSector.FirstFreeSector(); //-- 1st sector number of the data area (after the root directory for fat12/16)

    //-- 1st sector number of the data area.
    //-- for fat12/16 it will be 1st sector after the root directory
    //-- for fat32 it will be the 1st sector of the second cluster after the root directory. At present we don't handle the 1st bad cluster of the root dir.
    TUint32 dataStartSector;

    if(BootSector.FatType() == EFat32)
    {
        dataStartSector = BootSector.RootDirStartSector() + 1*BootSector.SectorsPerCluster();
    }
    else
    {
        dataStartSector = BootSector.FirstDataSector();
    }


    test.Printf(_L("data start sector:%d, total sectors:%d\n"), dataStartSector, BootSector.VolumeTotalSectorNumber());

     //-- randomly generate bad sectors numbers in the FAT volume data area
    for(TInt i = 0; i<size; ++i)
     {
        const TUint32 rand = ((TUint32)Math::Rand(rndSeed)) % (BootSector.VolumeTotalSectorNumber() - dataStartSector);
        const TUint32 sectNum = dataStartSector + rand;

        gBadList[i] = sectNum;
        test.Printf(_L("Value of gBadList[%d] is %d\n"), i, gBadList[i]);
     }

}


//-------------------------------------------------------------

/**
    Read and validate the boot sector
*/
static void ReadBootSector()
{
    test.Printf(_L("Reading Boot sector...\n"));

    TInt nRes = ReadBootSector(TheFs, gDrive, KBootSectorNum << KDefaultSectorLog2, BootSector);
    test(nRes == KErrNone);

    if(!BootSector.IsValid())
    {
        test.Printf(_L("The boot sector is invalid! dump:\n"));
        BootSector.PrintDebugInfo();
        test(0);
    }

}

//
// Write version number to disk using raw disk write
// For FAT 32 the version number is written to the backup boot sector as well.
//
void WriteVersionNumber()
{
    //-- boot sector version number is already corrupt in ChangeVersionNumberAndMountL()
    test.Printf(_L("writing boot sector version number.\n"));

    TInt nRes;

    //-- 1. corrupt a backup boot sector if it is on the volume. This will prevent
    //-- using a valid backup boot sector instead of corrupted main one.
    //-- we need to do it before corrupting the main one, because RawWrite causes volume remount.
    TFatBootSector bs1;
    const TUint32 KBkBootSecPos = KBkBootSectorNum << KDefaultSectorLog2;

    nRes = ReadBootSector(TheFs, gDrive, KBkBootSecPos, bs1);
    if(nRes == KErrNone && bs1.IsValid())
    {//-- write corrupted backup boot sector
        nRes = WriteBootSector(TheFs, gDrive, KBkBootSecPos, BootSector);
        test(nRes == KErrNone);
    }

    //-- write main boot sector from the pos 0
    nRes = WriteBootSector(TheFs, gDrive, KBootSectorNum << KDefaultSectorLog2, BootSector);
    test(nRes == KErrNone);
}

//
// Root Cluster Number update test
//
void DoTestRootClusterUpdate()
{
    test.Next(_L("Performing fat32 root cluster number update check\n"));

    //Mark cluster 2 as bad by raw access.
    RRawDisk rawDisk;

    TInt r=rawDisk.Open(TheFs,gSessionPath[0]-'A');
    test_KErrNone(r);

    //Mark Cluster 2  & 3 as bad
    const TInt fatStartPos = BootSector.FirstFatSector() * BootSector.BytesPerSector();
    TInt64 pos = 8 + fatStartPos;

    TBuf8<4> data(4);
    data[0] = 0xF7;
    data[1] = 0xFF;
    data[2] = 0xFF;
    data[3] = 0x0F;

    r=rawDisk.Write(pos, data);
    test_KErrNone(r);

    pos += 4;
    r = rawDisk.Write(pos, data);
    test_KErrNone(r);

    rawDisk.Close();

    //-- quick format the drive
    r = FormatFatDrive(TheFs, gDrive, ETrue);
    test_KErrNone(r);

    const TUint oldClusterNum = BootSector.RootClusterNum();
    ReadBootSector();

    test.Printf(_L("Old Rool clNum:%d, new:%d\n"), oldClusterNum, BootSector.RootClusterNum());

    test(BootSector.RootClusterNum() != oldClusterNum);

}

//
//Make version number invalid and try to mount
//
void ChangeVersionNumberAndMountL()
{
    test.Printf(_L("Change fat32 version number and remount\n"));

    TFullName name;

    BootSector.SetVersionNumber(0x707); //-- corrupt Version number in the boot sector

    WriteVersionNumber();

    TInt r = TheFs.FileSystemName(name, gDrive);
    test_KErrNone(r);

    r = DismountFileSystem(TheFs, name, gDrive);
    test_KErrNone(r);

    r = MountFileSystem(TheFs, name, gDrive);

    test_Value(r, r == KErrCorrupt);

}


//
// Test version number
//
void DoTestVersionNumber()
{
    test.Next(_L("Performing fat32 version number check\n"));
    TFullName name;
    TInt count;

    ReadBootSector();
    ChangeVersionNumberAndMountL();

    // Test quick format
    RFormat formatQuick;
    TInt r = formatQuick.Open(TheFs, gDriveName, EQuickFormat, count);
    test_KErrNone(r);

    r = DoFormatSteps(formatQuick, count);
    test_KErrNone(r);

    formatQuick.Close();

    ReadBootSector();
    r = TheFs.FileSystemName(name, gDrive);
    test_KErrNone(r);

    r = DismountFileSystem(TheFs, name, gDrive);
    test_KErrNone(r);

    r = MountFileSystem(TheFs, name, gDrive);
    test_KErrNone(r);

}

//
// Test format disk with bad clusters
//
void DoTestFormatL()
    {
    test.Next(_L("Test full format\n"));

    // Test full format
    TInt count;
    TInt r = KErrNone;

    RFormat formatFull;
    r = formatFull.Open(TheFs, gDriveName, EFullFormat, count);
    TESTL(r == KErrNone);

    // Insert bad clusters, based on the boot sector info obtained by the initial format
    MarkClustersL();

    r = DoFormatSteps(formatFull, count);
    TESTL(r == KErrNone);

    formatFull.Close();

    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EUnmarkAll, NULL, NULL);
    TESTL(r==KErrNone);

    test.Next(_L("Test quick format"));

    // Test quick format
    RFormat formatQuick;
    r = formatQuick.Open(TheFs, gDriveName, EQuickFormat, count);
    TESTL(r == KErrNone);

    r = DoFormatSteps(formatQuick, count);
    TESTL(r == KErrNone);

    formatQuick.Close();

    test.Next(_L("Test full format"));

    // Test full format
    RFormat formatFull2;
    r = formatFull2.Open(TheFs, gDriveName, EFullFormat, count);
    TESTL(r == KErrNone);
    MarkClustersL();

    r = DoFormatSteps(formatFull2, count);
    TESTL(r == KErrNone);

    formatFull2.Close();

    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EUnmarkAll, NULL, NULL);
    TESTL(r==KErrNone);

    test.Next(_L("Test user specific format"));

    // Test user specific format
    DoTestUserFormatL(EFalse, EFalse);
    ReadBootSector();
    CreateBadList();
    DoTestUserFormatL(EFalse, ETrue);

    r = TheFs.ControlIo(gDrive, CFatTestProxyDrive::EUnmarkAll, NULL, NULL);
    TESTL(r==KErrNone);

    test.Next(_L("Test user specific quick format"));

    // Test user specific quick format
    DoTestUserFormatL(ETrue, EFalse);
    }

void DoTestL()
    {
    // Make sure the card is formatted, then extract boot sector info

    TInt count;
    RFormat formatFull;

    //Mini SD cards works properly only with ESpecialFormat. Fix for Defect DEF091659
    TInt r = formatFull.Open(TheFs, gDriveName, ESpecialFormat, count);
    test_KErrNone(r);

    r = DoFormatSteps(formatFull, count);
    test_KErrNone(r);

    formatFull.Close();

    ReadBootSector();
    //...and create the bad sector list
    CreateBadList();

    DoTestReadWriteL();

    DoTestFormatL();
    }


//-------------------------------------------------------------

void CallTestsL()
    {
#if !(defined(_USE_CONTROLIO) || defined(_DEBUG) || defined(_DEBUG_RELEASE))
    test.Printf(_L("Tests only run in debug mode"));
    return;
#else
    // Only test FAT filesystem
    TInt r;
    r = TheFs.CharToDrive(gDriveToTest, gDrive);
    test_KErrNone(r);
    gDriveName[0] = (TText)gDriveToTest;
    gDriveName[1] = ':';

    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());

    //-- print drive information
    PrintDrvInfo(TheFs, gDrive);

    if (!Is_Fat(TheFs, gDrive))
        {
        test.Printf(_L("Drive %d is not FAT file system\n"),gDrive);
        return;
        }

    // No need for variable size disk
    TDriveInfo info;
    r = TheFs.Drive(info, gDrive);
    test_KErrNone(r);
    if (info.iMediaAtt & KMediaAttVariableSize)
        {
        test.Printf(_L("Drive %d is variable-size disk, skip\n"),gDrive);
        return;
        }

    //-- perform FAT32 specific tests

    if(Is_Fat32(TheFs, gDrive) && !Is_Automounter(TheFs, gDrive))
    {//-- these tests tend to corrupt the volume; Automounter can't cope with it easily
        DoTestVersionNumber();
        DoTestRootClusterUpdate();
    }

    //-- install test extension etc.
    if(!PrepareMount())
        return;

    //-- perform the rest of the tests
    DoTestL();

    //-- dismount the extension and format the drive
    //-- there is an issue here: if one of the tests fail, the extension won't be dismounted, which
    //-- will cause total crash.
    RestoreEnv();
#endif
    }




