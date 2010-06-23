// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\t_ext1.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"
#include "f32_test_utils.h"

using namespace F32_Test_Utils;


GLDEF_D RTest test(_L("T_EXT1"));

_LIT(KExtensionLog,"T_LOGEXT");
_LIT(KExtensionLogName,"Logger");
_LIT(KExtensionEmpty,"T_EMPTYEXT");
_LIT(KExtensionEmptyName,"Empty");
_LIT(KExtensionBit,"T_BITEXT");
_LIT(KExtensionBitName,"Bitchange");
_LIT(KExtensionRubbish,"T_RUBBISH");
_LIT(dir1,"\\dir1\\");
_LIT(file1Name,"\\dir1\\file1.doc");
_LIT8(toWrite,"abcdefghijklmnop");

void TestSecondaryExtensions()
//
// a secondary extension is one that is added to a drive with an existing file system
// therefore a mount is successful with or without the extension
//
	{
	test.Next(_L("TestSecondaryExtensions()"));
	TInt drive;
	TInt err=RFs::CharToDrive(gDriveToTest,drive);
	test_KErrNone(err);
	
	TPckgBuf<TBool> drvSyncBuf;
	err = TheFs.QueryVolumeInfoExt(drive, EIsDriveSync, drvSyncBuf);
	test_KErrNone(err);
	const TBool bDrvSync = drvSyncBuf();
			

	TFullName fsName;
	TInt r=TheFs.FileSystemName(fsName,drive);
	test_KErrNone(r);
	test.Printf(_L("fsName=%S\n"),&fsName);

	if (Is_SimulatedSystemDrive(TheFs, drive))
		{
		// check that the extension cannot be mounted since it is not supported by the file system
		test.Printf(_L("Test extension cannot be mounted"));
		r=TheFs.AddExtension(KExtensionLog);
		test_KErrNone(r);
		r=TheFs.MountExtension(KExtensionLogName,drive);
		test_Value(r, r == KErrNotSupported);
		r=TheFs.RemoveExtension(KExtensionLogName);
		test_KErrNone(r);
		return;
		}

	test.Next(_L("RFs::AddExtension()"));
	r=TheFs.AddExtension(KExtensionLog);
	RDebug::Print(_L("addext=%d"),r);
	test_KErrNone(r);
	r=TheFs.AddExtension(KExtensionLog);
	test_Value(r, r == KErrAlreadyExists);
	r=TheFs.AddExtension(KExtensionRubbish);
	test_Value(r, r == KErrNotFound);
	r=TheFs.AddExtension(KExtensionEmpty);
	test_KErrNone(r);

	test.Next(_L("RFs::MountExtension()"));
#if !defined(__WINS__)
	// check that the extension cannot be mounted on file system that does not support extensions
	r=TheFs.MountExtension(KExtensionLogName,EDriveZ);
	test_Value(r, r == KErrNotSupported);
#endif
	// test mounting on drive with no file system
	r=TheFs.DismountFileSystem(fsName,drive);
	test_KErrNone(r);
	r=TheFs.MountExtension(KExtensionLogName,drive);
	test_Value(r, r == KErrNotReady);
	r=TheFs.MountFileSystem(fsName,drive,bDrvSync);
	test_KErrNone(r);
	// test with a resource open
	_LIT(KFileName,"testing.doc");
	RFile file;
	r=file.Replace(TheFs,KFileName,EFileShareExclusive);
	test_KErrNone(r);
	r=TheFs.MountExtension(KExtensionLogName,drive);
	test_Value(r, r == KErrInUse);
	file.Close();
	r=TheFs.Delete(KFileName);
	test_KErrNone(r);
	// test with a format open
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(drive+'A');
	RFormat format;
	TInt count;
	r=format.Open(TheFs,driveBuf,EHighDensity,count);
	test_KErrNone(r);
	r=TheFs.MountExtension(KExtensionLogName,drive);
	test_Value(r, r == KErrInUse);
	format.Close();
	// get the extension name
	TFullName extName;
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNotFound);
	// now load the extension
	r=TheFs.MountExtension(KExtensionLogName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionLogName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);
	// try remounting the same extension
	r=TheFs.MountExtension(KExtensionLogName,drive);
	test_Value(r, r == KErrAlreadyExists);
	// mount a second extension
	r=TheFs.MountExtension(KExtensionEmptyName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionLogName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNone && extName==KExtensionEmptyName);
	
    // force a remount on a removable media and check that extensions both exist
    test.Printf(_L("Test forcing remount\n"));
    TDriveInfo info;
	r=TheFs.Drive(info,drive);
	if(info.iDriveAtt&KDriveAttRemovable)
		{
        const TInt KMediaRemountForceMediaChange = 0x00000001;


        TRequestStatus changeStatus;
        TheFs.NotifyChange(ENotifyAll, changeStatus);

        test.Printf(_L("Remounting the drive\n"), r);
        r = TheFs.RemountDrive(drive, NULL, (TUint) KMediaRemountForceMediaChange);
        test_Value(r, r == KErrNotReady || r == KErrNone);
        
        do
        {
        test.Printf(_L("Waiting for media change...\n"));
        User::WaitForRequest(changeStatus);

        r=TheFs.Drive(info,drive);
        test.Printf(_L("Drive() returned %d\n"), r);

        TheFs.NotifyChange(ENotifyAll, changeStatus);
        }
    while (r == KErrNotReady);
    TheFs.NotifyChangeCancel(changeStatus);

    User::After(1000000); //-- don't know why we need this, otherwise for WINS mediadriver returns -18



/*	
        //-- strange, but this variant caused Media Driver to always return -18 
        //-- and the media have become inaccessible forever (on H2). 
        //-- funny enough, but turning ON heavy logging from the drivers (PBUS & KLOCDRV) helped the problem

        test.Printf(_L("Force media change\n"));
    	RLocalDrive d;
		TBool flag=EFalse;
		r=d.Connect(1,flag);
		test_KErrNone(r);
		d.ForceMediaChange();
		d.Close();
//#if defined(__WINS__)
		// ??? seems to work, find out why
		//User::After(500000);
        User::After(2000000);
//#endif
*/
		}



	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionLogName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNone && extName==KExtensionEmptyName);

    test.Printf(_L("Accessing media...\n"));
	// and now do some file system operations
	TBuf8<16> readBuf;
	r=TheFs.MkDir(dir1);
    
    test.Printf(_L("res=%d\n"), r);


	test_Value(r, r == KErrNone||r==KErrAlreadyExists);
	RFile file1;
	r=file1.Replace(TheFs,file1Name,EFileShareExclusive);
	test_KErrNone(r);
	r=file1.Write(toWrite);
	test_KErrNone(r);
	r=file1.Read(0,readBuf);
	test(readBuf==toWrite);
	r=file1.SetSize(0);
	test_KErrNone(r);
	file1.Close();
	r=TheFs.Delete(file1Name);
	test_KErrNone(r);
	r=TheFs.RmDir(dir1);
	test_KErrNone(r);

	test.Next(_L("RFs::DismountExtension()"));
	// test with a resource open
	r=file.Replace(TheFs,KFileName,EFileShareExclusive);
	test_KErrNone(r);
	r=TheFs.DismountExtension(KExtensionLogName,drive);
	test_Value(r, r == KErrInUse);
	file.Close();
	r=TheFs.Delete(KFileName);
	test_KErrNone(r);
	// test with a format open
	r=format.Open(TheFs,driveBuf,EHighDensity,count);
	test_KErrNone(r);
	r=TheFs.DismountExtension(KExtensionLogName,drive);
	test_Value(r, r == KErrInUse);
	format.Close();
	// now dismount an extension
	r=TheFs.DismountExtension(KExtensionLogName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionEmptyName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);
	// try to dismount an extension that is not mounted
	r=TheFs.DismountExtension(KExtensionLogName,drive);
	test_Value(r, r == KErrNotFound);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionEmptyName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);
	// dismount the remaining extension
	r=TheFs.DismountExtension(KExtensionEmptyName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNotFound);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);

	test.Next(_L("RFs::RemoveExtension()"));
	r=TheFs.RemoveExtension(KExtensionLogName);
	test_KErrNone(r);
	r=TheFs.RemoveExtension(KExtensionEmptyName);
	test_KErrNone(r);
	}

void TestPrimaryExtensions()
//
// a primary extension is one that is added to a drive before a file system is mounted
// must be present for the mount to be successful (eg. FTL)
//
	{
	test.Next(_L("TestPrimaryExtensions()"));
	TInt drive;
	TInt err=RFs::CharToDrive(gDriveToTest,drive);
	test_KErrNone(err);

	if(Is_SimulatedSystemDrive(TheFs, drive))
	    {
		test.Printf(_L("Skipping TestPrimaryExtensions on PlatSim/Emulator drive %C:\n"), gSessionPath[0]);
		return;
	    }

	TPckgBuf<TBool> drvSyncBuf;
	err = TheFs.QueryVolumeInfoExt(drive, EIsDriveSync, drvSyncBuf);
	test_KErrNone(err);
    const TBool bDrvSync = drvSyncBuf();

	// don't test on ram drive since accesses memory directly
	TDriveInfo info;
	TInt r=TheFs.Drive(info,drive);
	if(info.iMediaAtt&KMediaAttVariableSize)
		return;

	TFullName fsName;
	r=TheFs.FileSystemName(fsName,drive);
	test_KErrNone(r);
	test.Printf(_L("fsName=%S\n"),&fsName);

	test.Next(_L("RFs::AddExtension()"));
	r=TheFs.AddExtension(KExtensionLog);
	test_KErrNone(r);
	r=TheFs.AddExtension(KExtensionEmpty);
	test_KErrNone(r);
	r=TheFs.AddExtension(KExtensionBit);
	test_KErrNone(r);

	test.Next(_L("RFs::MountFileSystem()"));
	// test with file system that already exists
	r=TheFs.MountFileSystem(fsName,KExtensionBitName,drive,bDrvSync);
	test_Value(r, r == KErrAccessDenied);
	// unmount drive and mount primary extension along with file system
	r=TheFs.DismountFileSystem(fsName,drive);
	test_KErrNone(r);

    //-- !! N.B this extension mangles data read/written ftom/to the media, for some file systems it is OK and mounting succeeds
    //-- for others - this will result in KErrCorrupt
	r=TheFs.MountFileSystem(fsName,KExtensionBitName,drive,bDrvSync);
	test_Value(r, r == KErrNone||r==KErrCorrupt);
	
    // and now format
	Format(drive);
	TFullName extName;
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionBitName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);
	
	// and now do some file system operations
	TBuf8<16> readBuf;
	r=TheFs.MkDir(dir1);
	test_Value(r, r == KErrNone||r==KErrAlreadyExists);
	RFile file1;
	r=file1.Replace(TheFs,file1Name,EFileShareExclusive);
	test_KErrNone(r);
	r=file1.Write(toWrite);
	test_KErrNone(r);
	r=file1.Read(0,readBuf);
	test(readBuf==toWrite);
	r=file1.SetSize(0);
	test_KErrNone(r);
	file1.Close();
	r=TheFs.Delete(file1Name);
	test_KErrNone(r);
	r=TheFs.RmDir(dir1);
	test_KErrNone(r);

	// add a secondary extension
	test.Printf(_L("RFs::MountExtension()"));
	r=TheFs.MountExtension(KExtensionLogName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionBitName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNone && extName==KExtensionLogName);
	// try to add the same extension
	r=TheFs.MountExtension(KExtensionBitName,drive);
	test_Value(r, r == KErrAlreadyExists);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionBitName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNone && extName==KExtensionLogName);
	// try to add a third extension
	r=TheFs.MountExtension(KExtensionEmptyName,drive);
	test_Value(r, r == KErrAccessDenied);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionBitName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNone && extName==KExtensionLogName);

	// and now do some file system operations
	r=TheFs.MkDir(dir1);
	test_Value(r, r == KErrNone||r==KErrAlreadyExists);
	r=file1.Replace(TheFs,file1Name,EFileShareExclusive);
	test_KErrNone(r);
	r=file1.Write(toWrite);
	test_KErrNone(r);
	r=file1.Read(0,readBuf);
	test(readBuf==toWrite);
	r=file1.SetSize(0);
	test_KErrNone(r);
	file1.Close();
	r=TheFs.Delete(file1Name);
	test_KErrNone(r);
	r=TheFs.RmDir(dir1);
	test_KErrNone(r);

	test.Printf(_L("RFs::DismountExtension()"));
	// test that can't dismount a primary extension via this method
	r=TheFs.DismountExtension(KExtensionLogName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionBitName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);
	r=TheFs.DismountExtension(KExtensionBitName,drive);
	test_Value(r, r == KErrAccessDenied);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionBitName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);
	
	test.Printf(_L("RFs::DismountFileSystem()"));
	r=TheFs.MountExtension(KExtensionLogName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNone && extName==KExtensionBitName);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNone && extName==KExtensionLogName);
	// and now dismount
	r=TheFs.DismountFileSystem(fsName,drive);
	test_KErrNone(r);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNotReady);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotReady);
	// remount the file system
	r=TheFs.MountFileSystem(fsName,drive,bDrvSync);
	test_Value(r, r == KErrNone||r==KErrCorrupt);
	r=TheFs.ExtensionName(extName,drive,0);
	test_Value(r, r == KErrNotFound);
	r=TheFs.ExtensionName(extName,drive,1);
	test_Value(r, r == KErrNotFound);
	Format(drive);

	test.Next(_L("RFs::RemoveExtension()"));
	r=TheFs.RemoveExtension(KExtensionLogName);
	test_KErrNone(r);
	r=TheFs.RemoveExtension(KExtensionEmptyName);
	test_KErrNone(r);
	r=TheFs.RemoveExtension(KExtensionBitName);
	test_KErrNone(r);
	}


GLDEF_C void CallTestsL()
//
// Do tests relative to the session path
//
	{
    //-- set up console output 
    F32_Test_Utils::SetConsole(test.Console()); 
	
    TInt drive;
	TInt err=RFs::CharToDrive(gDriveToTest,drive);
	test.Start(_L("Starting Test - T_EXT1"));
	test_KErrNone(err);

	// Check that the drive supports extensions.
	TBool extensionsSupported = EFalse;
	TPckg<TBool> dataBuf(extensionsSupported);
	err = TheFs.QueryVolumeInfoExt(drive,EFSysExtensionsSupported,dataBuf);
	test_KErrNone(err);
	if(!extensionsSupported)
	    {
        test.Printf(_L("Drive %C: does not support file sys extensions. Skipping T_EXT1."), gSessionPath[0]);
        test.End();
        test.Close();
        return;
	    }
	
    PrintDrvInfo(TheFs, drive);

//Do not run this test on the NAND drive, as
//this has the FTL mounted as a primary extension
//which causes the test to fail
#if defined(__WINS__)
	if (drive==EDriveU)
		return;
#else
	TDriveInfo driveInfo;
	TheFs.Drive(driveInfo,drive);
	if (driveInfo.iType == EMediaNANDFlash)
		{
		test.Printf(_L("Skipping T_EXT1 as drive %C: is NAND\n"), gSessionPath[0]);
		return;
		}
#endif
	TestSecondaryExtensions();

    //-- the t_bitext extension mangles data from the media, which may make FS mounting fail because of "corrupted" data. 
    //-- Then this step formats the media, automounter doesn't support it straightforward way
    if(!Is_Lffs(TheFs, drive) && !Is_Automounter(TheFs, drive))
		{
		TestPrimaryExtensions();
		}

	test.End();
	test.Close();
	}
