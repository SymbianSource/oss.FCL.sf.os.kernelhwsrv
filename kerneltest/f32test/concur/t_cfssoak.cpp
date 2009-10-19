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
//

//! @file f32test\concur\t_csfsoak.cpp

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include <f32dbg.h>

#include "t_server.h"
#include "t_chlffs.h"
#include "t_tdebug.h"
#include "t_cfssoak.h"

GLDEF_D RTest test(_L("T_CFSSOAK"));
GLDEF_D	RFs TheFs;

GLREF_D TPtrC gArgV[];
GLREF_D TInt  gArgC;

LOCAL_D TFullName gFsName;
LOCAL_D TFullName gFsName1;
LOCAL_D TFullName gFsName2;
LOCAL_D TFullName gOldFsName;
LOCAL_D TFullName gNewFsName;
LOCAL_D TBool     gNoMedia = ETrue;

_LIT(KFsFile,   "CFAFSDLY");
_LIT(KFsName,   "DelayFS");

LOCAL_D const TInt32 KSecond = 1000000;

LOCAL_D TChar gRemFsChr = 0;
LOCAL_D TInt  gRemFsDrv = 0;

LOCAL_D TChar gDrvChFill = 0;

LOCAL_D TInt  gNumRead = 0;
LOCAL_D TInt  gNumFill = 0;
LOCAL_D TInt  gNumRem  = 0;

GLDEF_D TExtension gPrimaryExtensions[KMaxDrives];

inline void TraceError(const char* aFile, TInt aLine, const TDesC& aStr, TInt aRsp)
//
// 'Helper' routine to output the file and line of an error as well as a string
// and a response value.
//
    {
    TBuf<256> fbuf;
    TPtrC8    fptr((const TUint8*)aFile);
    fbuf.Copy(fptr);
    RDebug::Print(_L("%S:%d  %S: r = %d"), &fbuf, aLine, &aStr, aRsp);
    }

#define TESTSTR(r,s,cond) if (!(cond)) { TraceError(__FILE__, __LINE__, (s), (r)); test(0); }
#define TESTLIT(r,s,cond) { _LIT(KStr,s); TESTSTR(r,KStr,cond); }
#define TESTRES(r,cond)   TESTLIT(r,"ERROR",cond)

LOCAL_C TChar MountTestFileSystem(TInt aDrive)
///
/// Mount a new CTestFileSystem on the drive under test.
/// @param aDrive Drive number (EDriveC etc.) to be used.
///
	{
	TInt r;
	// Check attributes
	TBuf<64> b;
	TChar c;
	r=TheFs.DriveToChar(aDrive,c);
	TESTLIT(r, "TheFs.DriveToChar", r==KErrNone);
		
	b.Format(_L("Mount test file system on %c:"),(TUint)c);
	test.Next(b);

	r=TheFs.AddFileSystem(KFsFile);
	TESTLIT(r, "TheFs.AddFileSystem", r==KErrNone || r==KErrAlreadyExists);
	
	r=TheFs.FileSystemName(gOldFsName,aDrive);
	TESTLIT(r, "TheFs.FileSystemName", r==KErrNone || r==KErrNotFound);

	TDriveInfo drv;
	r = TheFs.Drive(drv, gRemFsDrv);
	TESTLIT(r, "Drive()", r == KErrNone);

	gNoMedia = (drv.iType == EMediaUnknown || drv.iType == EMediaNotPresent);

	if (gOldFsName.Length() > 0)
		{
		r = TheFs.ExtensionName(gPrimaryExtensions[aDrive].iName, aDrive, 0);
		if (r == KErrNone)
			gPrimaryExtensions[aDrive].iExists = ETrue;

		TTest::Printf(_L("Dismount %C: %S"), (TUint)c, &gOldFsName);
		r=TheFs.DismountFileSystem(gOldFsName,aDrive);
		TESTLIT(r, "DismountFileSystem", r==KErrNone);
		}

	if (gPrimaryExtensions[aDrive].iExists == EFalse)
		r=TheFs.MountFileSystem(KFsName,aDrive);
	else
		r=TheFs.MountFileSystem(KFsName,gPrimaryExtensions[aDrive].iName,aDrive);

	TESTLIT(r, "TheFs.MountFileSystem", r==KErrNone);

	r=TheFs.FileSystemName(gNewFsName,aDrive);
	TESTLIT(r, "TheFs.FileSystemName", r==KErrNone);
		
	r = gNewFsName.CompareF(KFsName);
	TESTLIT(r, "gNewFsName.Compare", r==0);
		
	return c;
	}

LOCAL_C void UnmountFileSystem(TInt aDrive)
///
/// Dismount the filesystem and remount the original one.
/// @param aDrive Drive number (EDriveC etc.) to be unmounted.
///
	{
	TChar c;
	TInt r=TheFs.DriveToChar(aDrive,c);
	TESTLIT(r, "TheFs.DriveToChar", r==KErrNone);

	r=TheFs.DismountFileSystem(gNewFsName,aDrive);
	TESTLIT(r, "TheFs.DismountFileSystem", r==KErrNone);

	if (gNoMedia)
		{
		TTest::Printf(_L("No media on %C: so don't remount it"), (TUint)c);
		}
	else if (gOldFsName.Length() > 0)
		{
		TTest::Printf(_L("Mount    %C: %S"), (TUint)c, &gOldFsName);
		if (gPrimaryExtensions[aDrive].iExists == EFalse)
			r=TheFs.MountFileSystem(gOldFsName,aDrive);
		else
			r=TheFs.MountFileSystem(gOldFsName,gPrimaryExtensions[aDrive].iName,aDrive);
		TESTLIT(r, "MountFileSystem", r==KErrNone);
		}
	}

LOCAL_C TInt testReadOnly(TAny* /*aData*/)
///
/// Task to test read-only operations.
///
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TSoakReadOnly rdonly;
	TInt64 seed = 178543;
	TInt r = KErrNone;
	rdonly.ExcludeDrive(gRemFsDrv);
	while (r == KErrNone)
		{
		r = rdonly.ScanDrives(ETrue, Math::Rand(seed) % 90 + 10);
		TTest::PrintLock();
		TTest::Printf();
		TTest::Printf(_L("Read-only test results:\n"));
		TSoakStats::Print();
		rdonly.iDrives.Print(_L("Drives:"));
		rdonly.iDirs.Print(_L("Dirs:"));
		rdonly.iFiles.Print(_L("Files:"));
		rdonly.iReads.Print(_L("Reads:"));
		TTest::Printf();
		TTest::PrintUnlock();
		gNumRead++;
		}
	delete cleanup;
	TTest::Fail(HERE, r, _L("ScanDrives error"));
	return r;
	}

LOCAL_C TInt testFillEmpty(TAny* /*aData*/)
///
/// Task to repeatedly fill and clean a drive.
///
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TVolumeInfo info;
	TFullName   gFsName;
	TChar  drvch = 0;
	TInt   drive = 0;
	TInt64 free = 0;
	RFs    fs;
	TInt   r = fs.Connect();
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("can't connect to fileserver\n"));
	if (gDrvChFill)
		{
		// User specified the drive, so use that explicitly
		drvch = gDrvChFill;
		}
	else
		{
		// User didn't specify the drive, so scan through them looking for
		// likely candidates.  Don't use ROM or RAM drives, or LFFS ones, or
		// our own 'slow' file system.  Out of what's left, look for the one
		// with the least (but non-zero) free space.
		for (drive = EDriveA; drive <= EDriveZ; drive++)
			{
			if (drive != gRemFsDrv)
				{
				TDriveInfo d;
				TInt r;
				r = fs.Drive(d, drive);
				if (r == KErrNone && d.iDriveAtt != 0 &&
					!(d.iDriveAtt & KDriveAttRom) &&
					d.iMediaAtt != EMediaRam &&
					!IsFileSystemLFFS(fs, drive))
					{
					r = fs.Volume(info, drive);
					if (r == KErrNone)
						{
						if (free <= 0 || info.iFree < free)
							{
							r = fs.DriveToChar(drive, drvch);
							if (r != KErrNone)
								TTest::Fail(HERE, r, _L("DriveToChar(%d)"), drive);
							free = info.iFree;
							}
						}
					else
						{
						// not interested in drives which produce errors, they
						// probably don't really exist.
						}
					}
				else
					{
					// Drive doesn't exist, is a ROM drive or an LFFS drive, don't
					// use it.
					}
				}
			}
		}

	// If no drive found, can't do fill test so fail.
	if (drvch == 0)
		{
		TTest::Fail(HERE, _L("No drive found for fill test!"));
		}

	r = fs.CharToDrive(drvch, drive);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("CharToDrive(%C:)\n"), (TUint)drvch);
	r = fs.FileSystemName(gFsName, drive);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("FileSystemName(%C:)\n"), (TUint)drvch);

	r = fs.Volume(info, drive);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("Volume(%C:)\n"), (TUint)drvch);
	
	fs.Close();

	TTest::Printf(_L("Using %C: (%S) with %d KB free for fill/clean cycle test\n"),
		          (TUint)drvch, &gFsName, I64LOW(info.iFree / 1024));

	TSoakFill fill;
	r = fill.SetDrive(drvch);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("Unable to setup drive %C"), (TUint)drvch);

	r = fill.CleanDrive();
	if (r != KErrNone && r != KErrNotFound && r != KErrPathNotFound)
		TTest::Fail(HERE, r, _L("Error cleaning drive %C"), (TUint)drvch);

	// Loop to do the actual test
	while (r == KErrNone)
		{
		r = fill.FillDrive();
		if (r != KErrNone && r != KErrDiskFull)
			TTest::Fail(HERE, r, _L("Error filling drive %C"), (TUint)drvch);

		TTest::PrintLock();
		TTest::Printf(_L("%C: filled\n"), (TUint)drvch);
		TTest::PrintUnlock();

		r = fill.CleanDrive();
		if (r == KErrPathNotFound)
			r = KErrNone;
		if (r != KErrNone)
			TTest::Fail(HERE, r, _L("cleaning drive %C"), (TUint)drvch);

		TTest::PrintLock();
		TTest::Printf(_L("%C: cleaned\n"), (TUint)drvch);
		TTest::PrintUnlock();

		gNumFill++;
		}
	delete cleanup;
	TTest::Fail(HERE, r, _L("Fill/clean error"));
	return r;
	}

LOCAL_C TInt testRemote(TAny* /*aData*/)
///
/// Task to exercise the remote (special) filesystem.
///
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TSoakRemote rem(gRemFsChr);
	TInt r = KErrNone;
	for (TBool sync = EFalse; r == KErrNone; sync = !sync)
		{
		for (TInt i = 0; i < 1; i++)
			{
			rem.Remount(sync);
			r = rem.TestSession();
			if (r != KErrNone)
				TTest::Fail(HERE, r, _L("TestSession failed"));
			r = rem.TestSubSession();
			if (r != KErrNone)
				TTest::Fail(HERE, r, _L("TestSubSession failed"));
			r = rem.TestMount();
			if (r != KErrNone)
				TTest::Fail(HERE, r, _L("Mount/dismount failed"));
			User::After(100*1000000);
			gNumRem++;
			}
		}
	delete cleanup;
	TTest::Fail(HERE, r, _L("Test (remote) filesystem error"));
	return r;
	}

GLDEF_C void CallTestsL()
///
/// Do all tests.
///
	{
	// first thing, initialise local test class to make sure it gets done.
	TInt r = TTest::Init();
	test(r == KErrNone);
	
    const TInt KMaxArgs = 4;
    TPtrC argv[KMaxArgs];
    TInt  argc = TTest::ParseCommandArguments(argv, KMaxArgs);

	gRemFsChr = 'M'; // probably a 'safe' drive
	gDrvChFill = 0;

	if (argc > 1)
		gRemFsChr = User::UpperCase(argv[1][0]);
	if (argc > 2)
		gDrvChFill = User::UpperCase(argv[2][0]);

	r = TheFs.CharToDrive(gRemFsChr, gRemFsDrv);

	if (r != KErrNone || gRemFsDrv == EDriveZ)
		{
		test.Printf(_L("Test cannnot run on drive %C:\n"), (TUint)gRemFsChr);
		return;
		}

	// If we can do it, check whether the drives are LFFS and are OK
// !!! Disable platform security tests until we get the new APIs
/*	if (User::Capability() & KCapabilityRoot)
		{
		if (gDrvChFill != 0 && gDrvChFill != gRemFsChr)
			CheckMountLFFS(TheFs, gDrvChFill);
		}
*/
	MountTestFileSystem(gRemFsDrv);

	r = TTest::Create(0, testReadOnly, _L("T_ReadOnly"));
	test(r == KErrNone);

	r = TTest::Create(1, testFillEmpty, _L("T_FillEmpty"));
	test(r == KErrNone);

	r = TTest::Create(2, testRemote, _L("T_Remote"));
	test(r == KErrNone);

	TRequestStatus kStat;
	test.Console()->Read(kStat);
	TInt displaycount = 0;
	while ((r = TTest::Run(ETrue, 1*KSecond)) == KErrNone) // run until any thread exits
		{
		if (++displaycount >= 60 || kStat == KErrNone)
			{
			test.Printf(_L("Fill %-6d Rem %-6d Read %d\n"), gNumFill, gNumRem, gNumRead);
			displaycount = 0;
			}
		if (kStat == KErrNone)
			{
			test.Printf(_L("Aborted by user!\n"));
			break;
			}
		}
	test(r == KErrNone);

	// Kill all outstanding threads.
	TTest::KillAll(KErrAbort);

	UnmountFileSystem(gRemFsDrv);
	}

GLDEF_C TInt E32Main()
//
// Main entry point
//
    {
	for (TInt i = 0; i < KMaxDrives; i++)
		{
		gPrimaryExtensions[i].iExists = EFalse;
		}

    TInt r;
    CTrapCleanup* cleanup;
    cleanup=CTrapCleanup::New();
    __UHEAP_MARK;

    test.Title();
    test.Start(_L("Starting tests..."));

    r=TheFs.Connect();
    test(r==KErrNone);

    // TheFs.SetAllocFailure(gAllocFailOn);
    TTime timerC;
    timerC.HomeTime();

	// Do the test
    TRAP(r,CallTestsL());

    // reset the debug register
    TheFs.SetDebugRegister(0);

    TTime endTimeC;
    endTimeC.HomeTime();
    TTimeIntervalSeconds timeTakenC;
    r=endTimeC.SecondsFrom(timerC,timeTakenC);
    test(r==KErrNone);
    test.Printf(_L("Time taken for test = %d seconds\n"),timeTakenC.Int());
    // TheFs.SetAllocFailure(gAllocFailOff);
    TheFs.Close();
    test.End();
    test.Close();
    __UHEAP_MARKEND;
    delete cleanup;
    return(KErrNone);
    }
