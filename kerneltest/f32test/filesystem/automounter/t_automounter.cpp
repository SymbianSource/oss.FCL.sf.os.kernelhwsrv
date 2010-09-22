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
// 
// Testing "automounter" filesystem plugin functionality. 
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

#include "filesystem_fat.h"
#include "filesystem_automounter.h" 


#include "t_server.h"
#include "fat_utils.h"

using namespace Fat_Test_Utils;




RTest test(_L("T_Automounter"));

static TInt     gDriveNum=-1; ///< drive number we are dealing with

//-------------------------------------------------------------------
//-- the debug test property string can be used to control automounter in debug mode.
const TUid KThisTestSID={0x10210EB3}; ///< this EXE SID
const TUint KPropKey = 0; //-- property key

//-------------------------------------------------------------------
/** strings for the test property that will override estart.txt setting for the automounter */
//_LIT8(KSection,  "AutoMounter");        ///< section name
_LIT8(KKey_ChildFsList, "AM_FSNames");     ///< a key for the CSV list of child file system names
_LIT8(KProp_DefFmtFsIdx, "AM_DefFmtFsIdx");///< a key for the optional parameter that specifies the child file system index, which will be used for formatting unrecognised media


//-------------------------------------------------------------------
//-- Actually, for testing autoounter, it is neccessary to have at least 3 filesystems:
//-- automounter itself and any 2 dirrerent filesystems that can be used as child ones. 
//-- Let's use FAT as a 1st child, and exFAT as 2nd. All these 3  *.fsy shall be present.

/** automounter filesystem name */
#define KAutoMounterFSName KFileSystemName_AutoMounter
_LIT(KAutoMounterFsy,    "automounter.fsy");    ///< automounter *.fsy module name


//-- FAT is used as a child filesystem #0

/**  filesystem #1 name */
#define KFSName1 KFileSystemName_FAT

#if defined(__WINS__) //-- FAT fsy name is a mess.
_LIT(KFsy1, "efat32.fsy");
#else
_LIT(KFsy1, "elocal.fsy");
#endif    
    

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//-- exFAT is used as a child filesystem #1. The problem here: some poor guys might not have the exFAT at all including the header file
//-- "filesystem_exfat.h" that defines exFAT volume formatting structure. Fortunately for them the exFAT formatting parameters like "sectors per cluster" and
//-- "number of FATs" have the same layout in the data container as FAT ones. So FAT formatting structure can be used for formatting exFAT.
//-- The macro defines if exFAT might not be available.
#define EXFAT_MIGHT_NOT_BE_PRESENT

/** filesystem #2 name */
#ifdef EXFAT_MIGHT_NOT_BE_PRESENT
    _LIT(KFSName2, "exFAT");
#else
    #define KFSName2 KFileSystemName_exFAT
    #include "filesystem_exfat.h" 
    using namespace FileSystem_EXFAT;
#endif




_LIT(KFsy2, "exfat.fsy"); ///< filesystem #2 *.fsy module name

TBool automounter_Loaded  = EFalse; ///< ETrue if automounter.fsy is loaded; used for correct cleanup
TBool childFs1_Loaded     = EFalse; ///< ETrue if child #0 *.fsy is loaded; used for correct cleanup
TBool childFs2_Loaded     = EFalse; ///< ETrue if child #1 *.fsy is loaded; used for correct cleanup

TFSDescriptor   orgFsDescriptor; //-- keeps parameters of the original FS

//-------------------------------------------------------------------

/**
    perform some operations to see if the file system works at all
*/
void CheckFsOperations()
{
    TInt nRes;

    TVolumeInfo v;
    nRes = TheFs.Volume(v);
    test_KErrNone(nRes);

    _LIT(KTestDir, "\\directory1\\DIR2\\another directory\\");
    MakeDir(KTestDir);

    _LIT(KTestFile, "\\this is a file to test.bin");
    nRes = CreateCheckableStuffedFile(TheFs, KTestFile, 20376);
    test_KErrNone(nRes);

    nRes = VerifyCheckableFile(TheFs, KTestFile);
    test_KErrNone(nRes);

    nRes = TheFs.Delete(KTestFile);
    test_KErrNone(nRes);

    nRes = TheFs.CheckDisk(gSessionPath);
    test_KErrNone(nRes);

}

//-------------------------------------------------------------------

/**
    Check that FileSystem1 subtype matches one of the expected
*/
void CheckSubtype_FS1(const TDesC& aFsSubtype)
{
    _LIT(KFatSubType12, "fat12");
    _LIT(KFatSubType16, "fat16");
    _LIT(KFatSubType32, "fat32");

    test(aFsSubtype.CompareF(KFatSubType12) == 0 || aFsSubtype.CompareF(KFatSubType16) == 0 || aFsSubtype.CompareF(KFatSubType32) == 0 );
}

//-------------------------------------------------------------------

/**
    Check that FileSystem2 subtype matches expected
*/
void CheckSubtype_FS2(const TDesC& aFsSubtype)
{
    _LIT(KExFatSubType, "exFAT");
    test(aFsSubtype.CompareF(KExFatSubType) == 0);
}


//-------------------------------------------------------------------
/**
    Dismounts Currently mounted file system.
*/
static TInt DoDismountFS()
{
    TFSName fsName;
    TInt nRes;
   
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    if(nRes == KErrNone)
    {
        test.Printf(_L("--- Dismounting FS:%S\n"), &fsName);
        nRes = TheFs.DismountFileSystem(fsName, gDriveNum);
        return nRes;
    }

    return KErrNone; //-- no file system mounted
}

//-------------------------------------------------------------------
/**
    Dismounts Currently mounted file system.
*/
static void DismountFS()
{
    test(DoDismountFS() == KErrNone);
}

//-------------------------------------------------------------------
/**
    Dismounts Currently mounted file system.
*/
static void ForceDismountFS()
{
    test.Printf(_L("--- Force dismounting current FS\n"));
    TRequestStatus stat;
    TheFs.NotifyDismount(gDriveNum, stat, EFsDismountForceDismount); 
    User::WaitForRequest(stat);         
    //test(stat.Int() == KErrNone);
}



//-------------------------------------------------------------------
/**
    Mount the given file system. Mounting file system doesn't mean that it will be usable.
    For example, KErrCorrupt can be the result if FS doesn't recognise bootsectors etc. 

    @param  aFsName file system name
    @return error code
*/
static TInt DoMountFS(const TDesC& aFsName)
{
    TInt nRes;
    test.Printf(_L("+++ Mounting FS:%S\n"), &aFsName);

    TFSDescriptor   newFsDescriptor = orgFsDescriptor;
    newFsDescriptor.iFsName = aFsName;
    test(!newFsDescriptor.iDriveSynch); //-- mount the given FS as asynchronous one, the automounter can't be used on a synchronous drive anyway.

    nRes = MountFileSystem(TheFs, gDriveNum, newFsDescriptor);
    
    if(nRes != KErrNone)
    {
        test.Printf(_L("++> Error Mounting FS! code:%d\n"), nRes);
    }
    else
    {
        PrintDrvInfo(TheFs, gDriveNum);
    }


    return nRes;
}


//-------------------------------------------------------------------
/**
    Explicitly mount the "automounter" FS
*/
static void Mount_AutomounterFS()
{
    DismountFS();
    DoMountFS(KAutoMounterFSName);
}

//-------------------------------------------------------------------
/**
    Explicitly mount the FileSystem1
*/
static void Mount_FileSystem1()
{
    DismountFS();
    DoMountFS(KFSName1);

}

//-------------------------------------------------------------------
/**
    Explicitly mount the FileSystem2
*/
static void Mount_FileSystem2()
{
    DismountFS();
    DoMountFS(KFSName2);
}

//-------------------------------------------------------------------
/**
    Just fill first 32 sectors with zeroes.
    The volume will require formatting after this.
*/
static void CorruptDrive()
{
    TInt nRes;
    test.Printf(_L("!!! corrupting the drive...\n"));

    RRawDisk  rawDisk;
    nRes = rawDisk.Open(TheFs, gDriveNum);
    test(nRes == KErrNone);

    TBuf8<512>  sectorBuf(512);

    sectorBuf.FillZ();

    const TInt  KSectors = 32;
    TInt64      mediaPos = 0;

    for(TInt i=0; i<KSectors; ++i)
    {
        nRes = rawDisk.Write(mediaPos, sectorBuf);
        test(nRes == KErrNone);
        
        mediaPos += sectorBuf.Size();
    }

    rawDisk.Close();
}

//-------------------------------------------------------------------


/**
    quick format the volume using all parameter by default
*/
static void FormatVolume(TBool aQuickFormat = ETrue)
{
    TInt nRes;
    nRes = FormatDrive(TheFs, CurrentDrive(), aQuickFormat);
    test_KErrNone(nRes);
}


//-------------------------------------------------------------------
TInt DoFormatSteps(RFormat& aFormat, TInt& aFmtCnt)
{
    TInt nRes = KErrNone;

    while(aFmtCnt)
    {
        nRes = aFormat.Next(aFmtCnt);
        if(nRes != KErrNone)
        {
            test.Printf(_L("RFormat::Next() failed! code:%d\n"), nRes);
            break;
        }
    }

    return nRes;
}

//-------------------------------------------------------------------

/** 
    initialise test global objects 
    @return EFalse if something goes wrong
*/
TBool InitGlobals()
{
#ifndef _DEBUG
    test.Printf(_L("This test can't be performed in RELEASE mode! Skipping.\n"));
    test(0);
#endif
    
    TInt nRes;

    //-- store original file system parameters
    nRes = GetFileSystemDescriptor(TheFs, gDriveNum, orgFsDescriptor);
    test_KErrNone(nRes);


    //=======================================
    //-- define a text propery that will override automounter config string in estart.txt
    //-- automounter must be able to parse this string.
    //-- The property key is a drive number being tested
    {
        
        _LIT_SECURITY_POLICY_PASS(KTestPropPolicy);
        
        nRes = RProperty::Define(KThisTestSID, KPropKey, RProperty::EText, KTestPropPolicy, KTestPropPolicy);
        test(nRes == KErrNone || nRes == KErrAlreadyExists);

        //-- set the propery, it will override automounter config from estart.txt. 
        //-- the config string has following format: "<fs_name1>,<fsname2>"
        //-- and this looks like: 'FSNames fat, exfat'
        
    
        TBuf8<50> cfgBuf(0);
        cfgBuf.Append(KKey_ChildFsList);

        cfgBuf.Append(_L(" "));
        cfgBuf.Append(KFSName1);
        cfgBuf.Append(_L(" , "));
        cfgBuf.Append(KFSName2);
  
    
        nRes = RProperty::Set(KThisTestSID, KPropKey, cfgBuf);
        test_KErrNone(nRes);

    }
    
    //=======================================
    //-- we must ensure that all 3 required *.fsy are present and load them.
    //-- the automounter must have child filesystems loaded before its initialisation. 
    {
        _LIT(KFsyFailure, "can't load '%S', code:%d, the test can't be performed!\n");
        
        //-- child FS #0
        nRes = TheFs.AddFileSystem(KFsy1);
        if(nRes != KErrNone && nRes != KErrAlreadyExists)
        {
            test.Printf(KFsyFailure, &KFsy1, nRes);    
            return EFalse;
        }
        childFs1_Loaded = ETrue;


        //-- child FS #1
        nRes = TheFs.AddFileSystem(KFsy2);
        if(nRes != KErrNone && nRes != KErrAlreadyExists)
        {
            test.Printf(KFsyFailure, &KFsy2, nRes);    
            return EFalse;
        }
        childFs2_Loaded = ETrue;

        //-- automounter
        nRes = TheFs.AddFileSystem(KAutoMounterFsy);
        if(nRes != KErrNone && nRes != KErrAlreadyExists)
        {
            test.Printf(KFsyFailure, &KAutoMounterFsy, nRes);    
            return EFalse;
        }
        automounter_Loaded = ETrue;
    }


    //=======================================
    //-- dismount original file system and optional primary extension. Secondary extensions are not supported.

    test.Printf(_L("Dismounting the original FS:%S, PExt:%S \n"), &orgFsDescriptor.iFsName, &orgFsDescriptor.iPExtName);

    nRes = TheFs.DismountFileSystem(orgFsDescriptor.iFsName, gDriveNum);
    test_KErrNone(nRes);

    return ETrue;
}

//-------------------------------------------------------------------
/** destroy test global objects */
void DestroyGlobals()
{
    test.Printf(_L("Restoring the environment....\n"));

    TInt nRes;

    //=======================================
    //-- dismount current filesystem that was used for testing and mount the original filesystem
    if(orgFsDescriptor.iFsName.Length())
    {//-- the original file system had been dismounted during test initialisation; dismount whatever we have now
        test.Printf(_L("Mounting back the original FS:%S, PExt:%S \n"), &orgFsDescriptor.iFsName, &orgFsDescriptor.iPExtName);
        
        TFSName fsName;

        nRes = TheFs.FileSystemName(fsName, gDriveNum);
        if(nRes == KErrNone && fsName.CompareF(orgFsDescriptor.iFsName) != KErrNone)
        {
            nRes = TheFs.DismountFileSystem(fsName, gDriveNum);
            test_KErrNone(nRes);
        

            //-- mount original FS as asynchronous one, the automounter can't be used on a synchronous drive anyway.
            MountFileSystem(TheFs, gDriveNum, orgFsDescriptor);
       
            FormatVolume();
        }
    }

    //=======================================
    //-- delete test property
    RProperty::Delete(KThisTestSID, gDriveNum);

    //=======================================
    //-- if the original FS wasn't automounter, unload child file systems
    if(orgFsDescriptor.iFsName.CompareF(KFileSystemName_AutoMounter) != 0)
    { 

        if(childFs1_Loaded)
        {
            nRes = TheFs.RemoveFileSystem(KFSName1);
            test(nRes == KErrNone ||  nRes == KErrInUse); //-- if the FS was used on some drive before test started, It will be KErrInUse
            childFs1_Loaded = EFalse;
        }

        if(childFs2_Loaded)
        {
            nRes = TheFs.RemoveFileSystem(KFSName2);
            test(nRes == KErrNone ||  nRes == KErrInUse); //-- if the FS was used on some drive before test started, It will be KErrInUse
            childFs2_Loaded = EFalse;
        }
    

        //-- unload test filesystem modules
        if(automounter_Loaded)
        {
            nRes = TheFs.RemoveFileSystem(KAutoMounterFSName); //-- if the FS was used on some drive before test started, It will be KErrInUse
            test(nRes == KErrNone ||  nRes == KErrInUse);
            automounter_Loaded = EFalse;
        }
     }
     else
     {
        nRes = RemountFS(TheFs, gDriveNum);
        test(nRes == KErrNone);
     }

    TVolumeInfo v;
    TheFs.Volume(v);
}

//-------------------------------------------------------------------

/*
    Testing basic automounter functionality. Format media with different file systems FS1/FS2, mount automounter and 
    ensure that it recognised and successfully mounts appropriate child file system.

    
    1.1 mount "filesystem1" / format volume 
    1.2 check file system name / subtype / functionality
    1.3 corrupt "filesystem1"
    1.4 check  file system name / subtype


    2.1 mount "automounter"
    2.2 check file system name / subtype / functionality (this must correspond to the filesystem1) The "filesystem1" must be recognised

    3.1 mount "filesystem2" / format volume
    3.2 check file system name / subtype / functionality
    3.3 corrupt "filesystem2"
    3.4 check  file system name / subtype
    3.5 format volume (it will be filesystem2)

    4.1 mount "automounter"
    4.2 check file system name / subtype / functionality (this must correspond to the filesystem2) The "filesystem2" must be recognised
    
    
    5.  check the list of supported file systems on the drive

    6.1 corrupt the volume 
    6.2 check that automounter can't recognise it

    7.  restore "filesystem1" / format volume 
*/
void TestAutomounterBasics()
{
    
    test.Next(_L("Testing automounter basic functionality \n"));

    TVolumeInfo v;
    TFSName fsName(0);
    TFSName fsSubType(0);
    TInt nRes;
   
    //================================================================================
    //-- 1. mount "filesystem1" / format volume
    test.Printf(_L("Mounting FileSystem1...\n"));
    Mount_FileSystem1();
    
    FormatVolume();

    nRes = TheFs.Volume(v);
    test_KErrNone(nRes);
    
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test_KErrNone(nRes);
    test(fsName.CompareF(KFSName1) == 0);


    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    //-- check the list of supported file systems on this drive (there is only 1 FS supported).
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, -1);
    test(nRes == KErrArgument);
    
    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, RFs::KRootFileSystem); //-- "root" filesystem
    test(nRes == KErrNone && fsName.CompareF(KFSName1) == 0);

    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 0); //-- 1st "child" filesystem
    test(nRes == KErrNone && fsName.CompareF(KFSName1) == 0);

    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 1); //-- there are no more supported FSs
    test(nRes == KErrNotFound);

    //-- perform some operation
    CheckFsOperations();
    

    //================================================================================
    //-- 2. Now we have volume formatted for "filesystem1"; Mount "automounter" and check that the 
    //-- file system on the volume is recognised OK.
    test.Printf(_L("Mounting Automounter FS and checking its functionality ...\n"));
    Mount_AutomounterFS();

    nRes = TheFs.Volume(v);
    test_KErrNone(nRes);

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    
    test.Printf(_L("FS name:'%S', FS Subtype:'%S'\n") ,&fsName, &fsSubType);

    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    CheckSubtype_FS1(fsSubType);



    //================================================================================
    //-- dismount current file system
    test.Printf(_L("Dismomounting FileSystem1...\n"));
    DismountFS();
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(nRes == KErrNotFound);

    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, RFs::KRootFileSystem); //-- "root" filesystem
    test(nRes == KErrNotFound);

    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 0); //-- 1st "child" filesystem
    test(nRes == KErrNotFound);

    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);
    test(nRes == KErrNotReady);

    
    //================================================================================
    //-- 3. mount "filesystem2" / format volume
    test.Printf(_L("Mounting FileSystem2...\n"));
    Mount_FileSystem2();
    
    FormatVolume();

    nRes = TheFs.Volume(v);
    test_KErrNone(nRes);
    
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test_KErrNone(nRes);
    test(fsName.CompareF(KFSName2) == 0);

    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);
    test_KErrNone(nRes);
    CheckSubtype_FS2(fsSubType);

    //-- check the list of supported file systems on this drive (there is only 1 FS supported).
    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, RFs::KRootFileSystem); //-- "root" filesystem
    test(nRes == KErrNone && fsName.CompareF(KFSName2) == 0);

    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 0); //-- 1st "child" filesystem
    test(nRes == KErrNone && fsName.CompareF(KFSName2) == 0);

    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 1); //-- there are no more supported FSs
    test(nRes == KErrNotFound);

    //-- perform some operation
    CheckFsOperations();


    //================================================================================
    //-- 4. Now we have volume formatted for "filesystem2"; Mount "automounter" and check that the 
    //-- file system on the volume is recognised OK.
    test.Printf(_L("Mounting Automounter FS and checking its functionality ...\n"));
    Mount_AutomounterFS();

    nRes = TheFs.Volume(v);
    test_KErrNone(nRes);

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    
    test.Printf(_L("FS name:'%S', FS Subtype:'%S'\n") ,&fsName, &fsSubType);

    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    CheckSubtype_FS2(fsSubType);

    //================================================================================
    //-- 5. check the list of supported file systems on this drive (there must be 2 child FS supported).
    test.Printf(_L("Getting list of supported by automounter file systems ...\n"));
    
    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, RFs::KRootFileSystem); //-- "root" filesystem
    test(nRes == KErrNone && fsName.CompareF(KAutoMounterFSName) == 0);
    test.Printf(_L("Root FS:'%S'\n"), &fsName);


    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 0); //-- 1st "child" filesystem
    test(nRes == KErrNone && fsName.CompareF(KFSName1) == 0);

    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 1); //-- 2nd "child" filesystem
    test(nRes == KErrNone && fsName.CompareF(KFSName2) == 0);

    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 2); //-- 3rd "child" filesystem
    test(nRes == KErrNotFound);

    //-- get and print out list of all child FS (enumeration example)
    TInt i;
    for(i=0; ;++i)
    {
        nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, i); 
        if(nRes == KErrNone)
        {
            test.Printf(_L("child FS[%d]:'%S'\n"), i, &fsName);
        }
        else
        {
        test(nRes == KErrNotFound);
        break;    
        }
        
    }

    //-- perform some operation. They will happen on currently active child FS
    CheckFsOperations();

    //================================================================================
    //-- 6. corrupt the media, mount automounter, check that FS is not recognised.
    test.Printf(_L("Test automounter handling corrupted media.\n"));
    
    CorruptDrive(); //-- the active child FS will do this and the root FS will be remounted on first access

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype query requires mounted and recognised file system. this shall fail
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test(nRes == KErrCorrupt);

    nRes = TheFs.MkDir(_L("\\dir1\\"));
    test(nRes == KErrCorrupt);


    //================================================================================
    //-- 7. restore filesystem on the drive 
    test.Printf(_L("Restoring FileSystem1.\n"));
    Mount_FileSystem1();
    FormatVolume();
    
    nRes = TheFs.Volume(v);
    test_KErrNone(nRes);

}

//-------------------------------------------------------------------
/**
    
    1.1 mount "filesystem1" / format volume 
    1.2 create and open a file on the volume.
    1.3 dismount the file system with opened file on it.

    2.  mount "automounter" The "filesystem1" must be recognised
    2.1 open previously created file (it is still opened by dismounted FS1)

    3.  forcedly dismount the current file system (automounter)
    4.  mount the automounter FS again. check file system name / subtype; The "filesystem1" must be recognised
    5.  try to read a file (see 2.1), using already dismounted mount; it shall result in KErrDismounted.
*/
void TestDismounting()
{
    test.Next(_L("Testing media dismounting/remounting with automounter FS \n"));

    TInt nRes;
    TFSName    fsName(0);
    TFSName    fsSubType(0);
    TBuf8<40>   buf;

    //================================================================================
    //-- 1. mount "filesystem1" / format volume
    test.Printf(_L("Mounting FileSystem1 and opening a file.\n"));
    Mount_FileSystem1();
    FormatVolume();

    //-- create a file, open it and try to dismount FS
    _LIT(KTestFile, "\\test_file");
    nRes = CreateEmptyFile(TheFs, KTestFile, 100);
    test(nRes == KErrNone);
    
    RFile file;

    nRes = file.Open(TheFs, KTestFile, 0);
    test(nRes == KErrNone);

        //TheFs.SetDebugRegister(KFSERV);

    test.Printf(_L("dismounting FileSystem1 with a file opened.\n"));
    nRes = DoDismountFS();
    test(nRes == KErrInUse);

    file.Close();

    //================================================================================
    //-- 2. mount "automounter", previous FS must be recognised and set as an active child
    Mount_AutomounterFS();
   
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    //-- open the previously created file
    nRes = file.Open(TheFs, KTestFile, 0);
    test(nRes == KErrNone);

        //TheFs.SetDebugRegister(KFSERV);

    nRes = DoDismountFS();
    test(nRes == KErrInUse); //-- opened file belongs to the child FS, actually.


    //================================================================================
    //-- 3. force dismounting the file system, this will leave hanging dismounted mount associated with this drive.
    test.Printf(_L("Force dismounting the file system.\n"));
    ForceDismountFS();

        //TheFs.SetDebugRegister(KFSERV);

    //================================================================================
    //-- 4. mount "automounter" again, this will create another instance of mount corresponding to the filesystem1
    Mount_AutomounterFS();
   
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    //================================================================================
    //-- 5. try to read a file using already dead mount
    nRes = file.Read(0, buf, 2);
    test(nRes == KErrDisMounted);

    //-- this will cause the forcedly dismounted hanging mount to self-destruct
    file.Close();   


    //-- open the previously created file using current alive mount.
    nRes = file.Open(TheFs, KTestFile, 0);
    test(nRes == KErrNone);
    nRes = file.Read(0, buf, 2);
    test(nRes == KErrNone);

    file.Close();

    
    //TheFs.SetDebugRegister(0x00);
}

//-------------------------------------------------------------------
/**
    Testing legacy RFormat API in the case when the volume has "automounter" file system bound.
    The formatting is performed without specifying any parameters, i.e. "all by default"

    If the automounter recognises file system on the volume and successfully mounts it, the 
    default formatting must be transparent, i.e. the appropriate child FS will perform it.

    If the automounter can't recognise the filesystem on the volume because of volume corruption or if this FS is unknown to it,
    the "default" formatting will fail with "KErrNotFound"
*/
void TestAutomounterDefaultFormatting()
{
    test.Next(_L("Testing media formatting with default parameters. Automounter FS\n"));

    TInt nRes;
    TFSName    fsName(0);
    TFSName    fsSubType(0);


    //================================================================================
    //-- 1. mount "filesystem1" / format volume
    Mount_FileSystem1();
    FormatVolume();

    //================================================================================
    //-- 2. mount "automounter", previous FS must be recognised and set as an active child
    Mount_AutomounterFS();
   
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    //================================================================================
    //-- 3. format the drive with all default parameters; the current active child FS shall be used
    //-- check that we still have automounter as "root" FS and the same active child
    FormatVolume();

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    //-- perform some operations
    CheckFsOperations();
    test.Printf(_L("default formatting for FS:'%S', subtype:'%S' OK!\n"), &fsName, &fsSubType);

    //================================================================================
    //-- 3. mount "filesystem2" / format volume
    Mount_FileSystem2();
    FormatVolume();

    //================================================================================
    //-- 4. mount "automounter", previous FS must be recognised and set as an active child
    Mount_AutomounterFS();
   
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS2(fsSubType);

    //================================================================================
    //-- 5. format the drive with all default parameters; the current active child FS shall be used
    //-- check that we still have automounter as "root" FS and the same active child
    FormatVolume();

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS2(fsSubType);

    //-- perform some operations
    CheckFsOperations();
    test.Printf(_L("default formatting for FS:'%S', subtype:'%S' OK!\n"), &fsName, &fsSubType);

    //================================================================================
    //-- 6. corrupt the media, mount automounter, check that FS is not recognised.
    //-- default formatting shall fail, because automounter can't chose appropriate child FS
    
    CorruptDrive(); //-- the active child FS will do this and the root FS will be remounted on first access

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype query requires mounted and recognised file system. this shall fail
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test(nRes == KErrCorrupt);

    //-- try default formatting; this shall fail with a special error code
    nRes = FormatDrive(TheFs, CurrentDrive(), ETrue);
    test(nRes == KErrNotFound);

    //-- try special formatting without any parameters, it shall also fail.
    RFormat     format;
    TUint       fmtMode = EQuickFormat | ESpecialFormat;
    TInt        fmtCnt;
    TBuf<10>    drivePath;
    drivePath.Format(_L("%C:\\"), gDriveNum+'A');

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test(nRes==KErrNotFound);
    format.Close();



    //================================================================================
    //-- 7. restore filesystem on the drive, try formatting with specifying ESpecialFormat flag, but without any formatting parameters
    //-- just to check that it works (it will not work on SD cards that do not allow ESpecialFormat)
    test.Printf(_L("Restoring FileSystem1 and use special format without any parameters\n"));

    Mount_FileSystem1();
    //FormatVolume();
    

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test(nRes==KErrNone);

    nRes = DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();


    
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test_KErrNone(nRes);
    test(fsName.CompareF(KFSName1) == 0);


    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);


}



//-------------------------------------------------------------------
/**
    Testing the use case when there is "automounter" FS bound to the drive and RFormat API that allows specifying
    the file system name that we want to put on the volume.
    
    It must be possible:
        - format the volume without specifying FS name at all (the currently active child FS will be used)
        - format the volume with specifying FS name that belongs to one of the supported child FS. The volume shall be formatted with this FS.
        - format the volume with specifying incorrect FS name (not supported) the RFormat::Open() must fail with KErrNotSupported.

        - If the file system on the volume is damaged or not recognisable, the RFormat::Open() shall with KErrNotFound if the concrete file system name is not specified.
        - If the file system on the volume is damaged or not recognisable, if shall be possible to format such a volume by specifying the child FS name.
*/
void TestAutomounterFormatting_FsNameSpecified()
{
    test.Next(_L("Testing formatting API that allows specifying particular FS. Automounter FS.\n"));


    TInt nRes;
    TFSName    fsName(0);
    TFSName    fsSubType(0);
    
    TBuf<10>    drivePath;
    drivePath.Format(_L("%C:\\"), gDriveNum+'A');

    RFormat     format;
    TUint       fmtMode = EQuickFormat | ESpecialFormat;
    TInt        fmtCnt;

    TVolFormatParamBuf  fmtParamBuf;
    TVolFormatParam& fmtParam = fmtParamBuf();

    //_LIT(KTestFile, "\\this is a test file");

    //================================================================================
    //-- 0. prepare the volume
    Mount_FileSystem1();    
    FormatVolume(); //-- old API, formatting with all parameters by default


    //================================================================================
    //-- 0.1 mount "automounter", previous FS must be recognised and set as an active child
    Mount_AutomounterFS();
   
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    //================================================================================
    
    //-- 1.1 format the volume without specifying any parameters at all, the currently active child FS shall be used
    test.Printf(_L("format the volume without specifying any parameters at all\n"));

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); 

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    
    //-- 1.2 format the volume without specifying the FS name, the currently active child FS shall be used
    test.Printf(_L("format the volume without specifying the FS name\n"));
    fmtParam.Init(); //-- reset all data
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); 

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);


    CheckFsOperations();

    //================================================================================
    //-- 2. format the volume specifying _second_ child FS name
    test.Printf(_L("format the volume specifying second child FS name\n"));
    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(KFSName2);
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes =DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); 

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS2(fsSubType);
    
    CheckFsOperations();


    //================================================================================
    //-- 3. format the volume specifying _first_ child FS name
    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(KFSName1);
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes =DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); 

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);
    
    CheckFsOperations();


    //================================================================================
    //-- 4. try formatting the volume specifying wrond child FS name
    fmtParam.Init(); //-- reset all data
    
    fmtParam.SetFileSystemName(KAutoMounterFSName); //-- it might have some strange consequences :)
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotSupported);
    format.Close();

    fmtParam.SetFileSystemName(_L("wrong FS")); 
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotSupported);
    format.Close();


    //================================================================================
    //-- 5. corrupt the volume and try formatting without specyfying FS name
    CorruptDrive();

    fmtParam.Init(); //-- reset all data
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotFound); //-- the meaning: "can't find the appropriate file system to put onto the volume"
    format.Close();

    //test.Printf(_L("#### T_a #1 res:%d\n"), nRes);

    fmtParam.SetFileSystemName(KAutoMounterFSName); //-- it might have some strange consequences :)
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    //test.Printf(_L("#### T_a #2 res:%d\n"), nRes);
    
    test(nRes == KErrNotSupported || nRes==KErrNotFound);

    format.Close();

    fmtParam.SetFileSystemName(_L("wrong FS")); 
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotSupported);
    format.Close();

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test(nRes==KErrNotFound); //-- the meaning: "can't find the appropriate file system to put onto the volume"
    format.Close();


    //--------------------------------------------------------------------------------
    //-- 5.1 format the volume with specifying child FS2 explicitly
    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(KFSName2);
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes =DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); 

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS2(fsSubType);
    
    CheckFsOperations();

    //--------------------------------------------------------------------------------
    //-- 5.2 corrupt the volume and format with specifying child FS1 explicitly
    CorruptDrive();

    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(KFSName1);
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes =DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); 

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);
    
    CheckFsOperations();
}

//-------------------------------------------------------------------
/**
    Testing the use case when we have some rigidly bound FS to the drive (e.g "FAT")
    and RFormat API that allows specifying the file system name that we want to put on the volume.
    
    It must be possible:
        - format the volume without specifying FS name at all (the bound FS will be used)
        - format the volume without specifying FS name at all when the volume is corrupted (the bound FS will be used)    
        - format the volume with specifying FS name that is the same as the bound FS has.

    If the specified file system name differs from the name that the bound FS has, the RFormat::Open() fails with KErrNotSupported.

*/
void TestFixedFsFormatting_FsNameSpecified()
{
    test.Next(_L("Testing RFormat API that allows specifying particular FS name for fixed FS.\n"));

    TInt nRes;
    TFSName    fsName(0);
   
    TBuf<10>    drivePath;
    drivePath.Format(_L("%C:\\"), gDriveNum+'A');

    RFormat     format;
    TUint       fmtMode = EQuickFormat | ESpecialFormat;
    TInt        fmtCnt;

    TVolFormatParamBuf  fmtParamBuf;
    TVolFormatParam& fmtParam = fmtParamBuf();

    _LIT(KTestFile, "\\this is a test file");

    //================================================================================
    //-- 0. prepare the volume
    test.Printf(_L("fmt: ESpecialFormat, no parameters specified\n"));
    Mount_FileSystem1();    
    
    //-- 0.1 format the volume with ESpecialFormat and without any parameters at all
    test(fmtMode & ESpecialFormat);
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);


    format.Close();

    //================================================================================
    //-- 1. format the volume with default parameters without specifying file system name. The volume is already formatted with FS1
    test.Printf(_L("fmt: ESpecialFormat, no FS name specified #1\n"));
    fmtParam.Init(); //-- reset all data
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes =DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KFSName1) == 0); 
    CheckFsOperations();

    //-- 1.1 corrupt the media and check formatting without specifying FS name. 
    //-- The file system bount to this drive shall be used.
    CorruptDrive();
    
    test.Printf(_L("fmt: ESpecialFormat, no FS name specified #2\n"));

    fmtParam.Init(); //-- reset all data
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes =DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KFSName1) == 0); 
    CheckFsOperations();

    //-- 1.2 the media is already formatted with FS1, try to specify other file system name for formatting.
    //-- this shall fail with KErrNotSupported, the volume must not be affected
    test.Printf(_L("fmt: ESpecialFormat, specifying wrong FS name #1\n"));

    nRes = CreateCheckableStuffedFile(TheFs, KTestFile, 17384);
    test(nRes==KErrNone);

    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(_L("some filesystem name"));

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotSupported);
    format.Close();

    test.Printf(_L("fmt: ESpecialFormat, specifying wrong FS name #2\n"));
    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(KFSName2);

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotSupported);
    format.Close();
    
    
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KFSName1) == 0); 
    CheckFsOperations();

    nRes = VerifyCheckableFile(TheFs, KTestFile);
    test(nRes==KErrNone);


    //-- 1.3 corrupt the media and check formatting with the FS Name that doesn't match the FS bound to this drive
    //-- this shall fail
    test.Printf(_L("fmt: ESpecialFormat, specifying wrong FS name #3\n"));
    CorruptDrive();

    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(_L("some filesystem name"));

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotSupported);
    format.Close();

    test.Printf(_L("fmt: ESpecialFormat, specifying wrong FS name #4\n"));
    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(KFSName2);

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNotSupported);
    format.Close();

    //-- 1.4 specify the correct file system name (bound to this drive) formatting must succeed 
    test.Printf(_L("fmt: ESpecialFormat, specifying correct FS name\n"));
    fmtParam.Init(); //-- reset all data
    fmtParam.SetFileSystemName(KFSName1);
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KFSName1) == 0); 
    CheckFsOperations();


}

//-------------------------------------------------------------------
/**
    Test formatting FAT file system with some specific parameters, like sector per cluster etc.
    Note that some media types (like SD cards) do not support such type of formatting. 
*/
void TestFormatting_FsName_Parameters_FAT()
{
    using namespace FileSystem_FAT;

    test.Next(_L("Testing TVolFormatParam_FAT formatting API\n"));

    TInt nRes;
    TFSName    fsName(0);
   
    TBuf<10>    drivePath;
    drivePath.Format(_L("%C:\\"), gDriveNum+'A');

    RFormat     format;
    TUint       fmtMode = EQuickFormat | ESpecialFormat;
    TInt        fmtCnt;

    //================================================================================
    //-- 0. prepare the volume
    Mount_FileSystem1();    
    FormatVolume(); //-- old API, formatting with all parameters by default

    //================================================================================
    //-- 1.0  simple unit test for TVolFormatParam_FAT
    TVolFormatParam_FATBuf  fmtParamBuf_FAT;
    TVolFormatParam_FAT&    fmtParam_FAT = fmtParamBuf_FAT();

    fmtParam_FAT.SetFatSubType(EFat32);
    test(fmtParam_FAT.FatSubType() == EFat32);
    
    fmtParam_FAT.SetFatSubType(EFat12);
    test(fmtParam_FAT.FatSubType() == EFat12);
     
    fmtParam_FAT.SetFatSubType(EFat16);
    test(fmtParam_FAT.FatSubType() == EFat16);

    fmtParam_FAT.SetFatSubType(ENotSpecified);
    test(fmtParam_FAT.FatSubType() == ENotSpecified);

    fmtParam_FAT.SetFatSubType(KFSSubType_FAT32);
    test(fmtParam_FAT.FatSubType() == EFat32);
    
    fmtParam_FAT.SetFatSubType(KFSSubType_FAT12);
    test(fmtParam_FAT.FatSubType() == EFat12);
     
    fmtParam_FAT.SetFatSubType(KFSSubType_FAT16);
    test(fmtParam_FAT.FatSubType() == EFat16);

    
    fmtParam_FAT.SetSectPerCluster(64);
    test(fmtParam_FAT.SectPerCluster()==64);

    fmtParam_FAT.SetNumFATs(1);
    test(fmtParam_FAT.NumFATs()==1);

    fmtParam_FAT.SetReservedSectors(13);
    test(fmtParam_FAT.ReservedSectors()==13);

    
    fmtParam_FAT.Init();
    test(fmtParam_FAT.FatSubType() == ENotSpecified);
    test(fmtParam_FAT.SectPerCluster() == 0);
    test(fmtParam_FAT.NumFATs()==0);
    test(fmtParam_FAT.ReservedSectors()==0);

    
    //--- formatting FAT without specifying any parameters. This shall always succeed
    test.Printf(_L("fmt: using TVolFormatParam_FAT, no parameters.\n"));

    fmtParam_FAT.Init();

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf_FAT);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    //-- formatting FAT with specifying some parameters. This may fail because the media doesn't support overriding FAT parameters
    //-- or because this parameters combination isn't compatible with the volume geometry.

    test.Printf(_L("fmt: using TVolFormatParam_FAT, some FAT specific parameters.\n"));
    fmtParam_FAT.SetFatSubType(EFat32);
    fmtParam_FAT.SetSectPerCluster(1);
    fmtParam_FAT.SetNumFATs(1);
    fmtParam_FAT.SetReservedSectors(13);

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf_FAT);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    if(nRes != KErrNone)
    {
        test.Printf(_L("formatting failed. reason code:%d\n"), nRes);        
    }

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KFSName1) == 0); 

    CheckFsOperations();

}

//-------------------------------------------------------------------
/**
    Test formatting FAT file system with some specific parameters, like sector per cluster etc.
    Note that some media types (like SD cards) do not support such type of formatting. 
*/

void TestFormatting_FsName_Parameters_exFAT()
{


    test.Next(_L("Testing TVolFormatParam_exFAT formatting API\n"));

    TInt nRes;
    TFSName    fsName(0);
   
    TBuf<10>    drivePath;
    drivePath.Format(_L("%C:\\"), gDriveNum+'A');

    RFormat     format;
    TUint       fmtMode = EQuickFormat | ESpecialFormat;
    TInt        fmtCnt;


    //================================================================================
    //-- 0. prepare the volume
    Mount_FileSystem2();    
    FormatVolume(); //-- old API, formatting with all parameters by default

    //================================================================================
    //-- 1.0  simple unit test for TVolFormatParam_FAT

#ifndef EXFAT_MIGHT_NOT_BE_PRESENT    
    TVolFormatParam_exFATBuf    fmtParamBuf;
    TVolFormatParam_exFAT&      fmtParam = fmtParamBuf();
#else
    //-- see the comments to "EXFAT_MIGHT_NOT_BE_PRESENT" macro definition
    TVolFormatParam_FATBuf  fmtParamBuf;
    TVolFormatParam_FAT&    fmtParam= fmtParamBuf();
#endif


    fmtParam.SetSectPerCluster(64);
    test(fmtParam.SectPerCluster()==64);

    fmtParam.SetSectPerCluster(14);
    test(fmtParam.SectPerCluster()==14);

    fmtParam.SetNumFATs(1);
    test(fmtParam.NumFATs()==1);

    fmtParam.SetNumFATs(2);
    test(fmtParam.NumFATs()==2);

    fmtParam.Init();
    test(fmtParam.SectPerCluster() == 0);
    test(fmtParam.NumFATs()==0);


    //--- formatting exFAT without specifying any parameters. This shall always succeed
    test.Printf(_L("fmt: using TVolFormatParam_exFAT, no parameters.\n"));
    fmtParam.Init();
    
#ifdef EXFAT_MIGHT_NOT_BE_PRESENT
    //-- need to forcedly set exFAT FS name, because fmtParam.Init(); set it to "FAT"
    ((TVolFormatParam&)fmtParam).SetFileSystemName(KFSName2);
#endif        

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test(nRes==KErrNone);

    format.Close();

    //-- formatting exFAT with specifying some parameters. This may fail because the media doesn't support overriding FAT parameters
    //-- or because this parameters combination isn't compatible with the volume geometry.

    test.Printf(_L("fmt: using TVolFormatParam_exFAT, some exFAT specific parameters.\n"));

    fmtParam.SetSectPerCluster(1);
    fmtParam.SetNumFATs(2);

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt, fmtParamBuf);
    test(nRes==KErrNone);
    
    nRes = DoFormatSteps(format, fmtCnt);
    if(nRes != KErrNone)
    {
        test.Printf(_L("formatting failed. reason code:%d\n"), nRes);        
    }

    format.Close();

    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KFSName2) == 0); 

    CheckFsOperations();
}

//-------------------------------------------------------------------
/**
    A helper method. Unloads and loads back automounter FSY plugin.
    This makes it re-parse its config either from estart.txt or from the debug property if it is set.
*/
void  MakeAutomounterReparseConfig()
{
    //-- make automounter re-parse the config property
    ForceDismountFS();

    TInt nRes = TheFs.RemoveFileSystem(KAutoMounterFSName);
    test(nRes == KErrNone);

    nRes = TheFs.AddFileSystem(KAutoMounterFsy);
    test(nRes == KErrNone);
}


//-------------------------------------------------------------------
/**
    Test how the automounter supports only 1 child FS configured.
    Everything should be the same as in the case with multiple childs, apart from the use case
    of formatting the unrecognisable media.

    In this case such formatting should succeed, because the one and only child FS will be used for this        
*/
void TestHandlingOneChildFS()
{
    test.Next(_L("Testing automounter with the only 1 Child FS bound\n"));

    TInt nRes;
    TBuf8<50> cfgBuf(0);
    
    //================================================================================
    //-- make automounter configuration property that has only one child FS 
    //-- and this looks like: 'FSNames fat'
    
    cfgBuf.Append(KKey_ChildFsList);

    cfgBuf.Append(_L(" "));
    cfgBuf.Append(KFSName1);
  
    nRes = RProperty::Set(KThisTestSID, KPropKey, cfgBuf);
    test_KErrNone(nRes);

    //-- make automounter re-parse the config property
    MakeAutomounterReparseConfig();

    //================================================================================
    //-- 1. prepare the volume
    Mount_FileSystem1();    
    FormatVolume(); 

    TFSName    fsName;
    TFSName    fsSubType;


    //================================================================================
    //-- 2. mount "automounter", previous FS must be recognised and set as an active child
    Mount_AutomounterFS();
   
    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);

    //================================================================================
    //-- 3. check the list of supported file system names
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, RFs::KRootFileSystem); //-- "root" filesystem
    test(nRes == KErrNone && fsName.CompareF(KAutoMounterFSName) == 0);
    test.Printf(_L("Root FS:'%S'\n"), &fsName);

    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 0); //-- 1st "child" filesystem
    test(nRes == KErrNone && fsName.CompareF(KFSName1) == 0);

    fsName.SetLength(0);
    nRes = TheFs.SupportedFileSystemName(fsName, gDriveNum, 1); //-- 2nd "child" filesystem can't be found, it doesn't exist in config
    test(nRes == KErrNotFound);

    //================================================================================
    //-- 4. corrupt the media, check that FS is not recognised.
    CorruptDrive(); 

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype query requires mounted and recognised file system. this shall fail
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test(nRes == KErrCorrupt);

    //================================================================================
    //-- 5. format the volume, this must be OK, because there is only 1 child file system
    FormatVolume();
    CheckFsOperations();

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    CheckSubtype_FS1(fsSubType);



}

//-------------------------------------------------------------------
/**
    A helper method that does the real job:
         - makes automounter config in a test property, specifying child FS for formatting unrecognisable media
         - corrupts the drive
         - format the drive w/o specifying the child FS
         - check that the child FS from the config had been used for formatting


*/
void DoTestDefaultChildForFormatting(TInt aChildIdx)
{

    TInt nRes;
    TBuf8<50> cfgBuf(0);
    
    //================================================================================
    //-- make automounter configuration property that 2 child FS and explicitly specifies a child FS for formatting unrecognisable media
    //-- and this looks like: 'FSNames fat, exfat'
    //--                       DefFmtFsIdx 1 

    cfgBuf.Append(KKey_ChildFsList);

    cfgBuf.Append(_L(" "));
    cfgBuf.Append(KFSName1);
    cfgBuf.Append(_L(" , "));
    cfgBuf.Append(KFSName2);

    cfgBuf.AppendFormat(_L8("\n%S %d"), &KProp_DefFmtFsIdx(), aChildIdx);
    
    nRes = RProperty::Set(KThisTestSID, KPropKey, cfgBuf);
    test_KErrNone(nRes);

    //-- make automounter re-parse the config property
    MakeAutomounterReparseConfig();


    //================================================================================
    //-- 1. mount "automounter"
    Mount_AutomounterFS();

    TFSName    fsName;
    TFSName    fsSubType;

    //================================================================================
    //-- 2. corrupt the media, check that FS is not recognised.
    CorruptDrive(); 

    //-- check file system name / subtype etc.
    nRes = TheFs.FileSystemName(fsName, gDriveNum);
    test(fsName.CompareF(KAutoMounterFSName) == 0); //-- the file system name shall be "automounter" - it is a root FS
    test_KErrNone(nRes);

    //-- the FS Subtype query requires mounted and recognised file system. this shall fail
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test(nRes == KErrCorrupt);


    //================================================================================
    //-- 3. format the volume, this must be OK, because the child FS for corrupting unrecognised media is specified 
    FormatVolume();
    CheckFsOperations();

    //-- the FS Subtype must be the subtype of the recognised child FS
    nRes = TheFs.FileSystemSubType(gDriveNum, fsSubType);   
    test_KErrNone(nRes);
    
    if(aChildIdx == 0)  //-- child idx=0, see cfgBuf
        CheckSubtype_FS1(fsSubType);
    else if(aChildIdx == 1) //-- child idx=1, see cfgBuf1
        CheckSubtype_FS2(fsSubType);
    else 
        test(0);
        



}

//-------------------------------------------------------------------
/**
    test a special case of formatting unrecognisable media, when automounter is configured to use 
    some explicit child FS for this
*/
void TestDefaultChildForFormatting()
{
    test.Next(_L("Test automounter formatting unrecognisable media when child FS is explicitly specified\n"));

    const TInt KDefFmtChild_FAT   = 0; //-- child FS#0 FAT
    const TInt KDefFmtChild_ExFAT = 1; //-- child FS#1 exFAT

    DoTestDefaultChildForFormatting(KDefFmtChild_FAT);
    DoTestDefaultChildForFormatting(KDefFmtChild_ExFAT);

    //-- do this test to check that this particular config hasn't broken automounter functionality
    TestFixedFsFormatting_FsNameSpecified();
}

//-------------------------------------------------------------------

void CallTestsL()
    {

    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());

#ifndef _DEBUG
    //-- automounter has a special debug interface allowing to control child file ssytems mounting in _DEBUG mode only
    test.Printf(_L("Skipping the test in the Release build! \n"));
    return;
#else

    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDriveNum);
    test(nRes==KErrNone);


    //-------------------------------------

    PrintDrvInfo(TheFs, gDriveNum);

    TVolumeInfo v;
    nRes = TheFs.Volume(v);
    test(nRes==KErrNone);
    if(v.iDrive.iMediaAtt & KMediaAttVariableSize)
        {
        test.Printf(_L("Skipping. Internal ram drive not tested.\n"));
        return;
        }

    if(v.iDrive.iType != EMediaHardDisk || !(v.iDrive.iDriveAtt & KDriveAttRemovable)) 
        {
        test.Printf(_L("The drive shall be removable and the media type EMediaHardDisk. Skipping.\n"));
        return;
        }

    
    //-------------------------------------

    if(InitGlobals())
        {//-- do tests here
    
          TestAutomounterBasics();
          TestDismounting(); 
          TestFixedFsFormatting_FsNameSpecified();  

          TestAutomounterDefaultFormatting();
          TestAutomounterFormatting_FsNameSpecified();

          TestFormatting_FsName_Parameters_FAT();
          TestFormatting_FsName_Parameters_exFAT();
        
        //-- these 2 tests must be the last ones before calling DestroyGlobals()
        //-- they fiddle with the automounter config and may affect following tests.
        TestHandlingOneChildFS();    
        TestDefaultChildForFormatting();

        }
    //-------------------------------------

    DestroyGlobals();
#endif
    }


















