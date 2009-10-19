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
#include "f32_test_utils.h"

using namespace F32_Test_Utils;




GLDEF_D RTest test(_L("T_CFSTEST"));

LOCAL_D TFullName gFsName;
LOCAL_D TFullName gFsName1;
LOCAL_D TFullName gFsName2;
LOCAL_D TFullName gOldFsName;
LOCAL_D TFullName gNewFsName;
LOCAL_D TBool     gNoMedia = ETrue;

_LIT(KFsFile,   "CFAFSDLY");
_LIT(KFsName,   "DelayFS");

LOCAL_D const TInt32 KSecond = 1000000;
LOCAL_D const TInt32 KTenthS = KSecond/10;
LOCAL_D const TInt   KNumBuf = 10;
LOCAL_D const TInt   KBufLen = 50000;

LOCAL_D TInt KSessionWaitSlow = 50 * KTenthS;

LOCAL_D TBuf8<KBufLen> gBuff[KNumBuf];
LOCAL_D TRequestStatus gStat[KNumBuf];

LOCAL_D TChar gRemFsChr = 0;
LOCAL_D TInt  gRemFsDrv = 0;

LOCAL_D TChar gDrvCh0 = 0;
LOCAL_D TChar gDrvCh1 = 0;
LOCAL_D TChar gDrvCh2 = 0;

LOCAL_D	TInt  gWaitTime = KSessionWaitSlow;

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
	TESTLIT(r, "DriveToChar", r==KErrNone);

	b.Format(_L("Mount test file system on %c:"),(TUint)c);
	test.Next(b);

	r=TheFs.AddFileSystem(KFsFile);
	TESTLIT(r, "AddFileSystem", r==KErrNone || r==KErrAlreadyExists);

	r=TheFs.FileSystemName(gOldFsName,aDrive);
	TESTLIT(r, "FileSystemName", r==KErrNone || r==KErrNotFound);

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

	TESTLIT(r, "MountFileSystem", r==KErrNone);

	r=TheFs.FileSystemName(gNewFsName,aDrive);
	TESTLIT(r, "FileSystemName", r==KErrNone);
		
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
	TESTLIT(r, "DriveToChar", r==KErrNone);

	if (gNewFsName.Length() > 0)
		{
		TTest::Printf(_L("Dismount %C: %S"), (TUint)c, &gNewFsName);
		r=TheFs.DismountFileSystem(gNewFsName,aDrive);
		TESTLIT(r, "DismountFileSystem", r==KErrNone);

		// if there's no media present, don't try to mount it
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
	}

LOCAL_C TInt TestCalibrate(TChar aDrvCh)
///
/// Calibrate the timing for writing the data buffers.  This also sets up
/// the buffers to the appropriate length (fast file systems such as LFFS
/// and FAT need big buffers, the slow test filesystem needs small ones).
///
    {
	TFileName name;
	RFile file;
	RFs   fs;
	TBool fast = (aDrvCh != gRemFsChr);
	TInt  drive;
	TInt  r;

	fs.Connect();

	name.Format(_L("%C:\\cfstest.txt"), (TUint)aDrvCh);

	r=fs.CharToDrive(aDrvCh, drive);
	TESTLIT(r, "CharToDrive", r==KErrNone);

	TVolumeInfo info;
	r = fs.Volume(info, drive);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("Getting volume info"));
	TTest::Printf(_L("Total space on drive %C = %ld (0x%lX)"), (TUint) aDrvCh, info.iSize, info.iSize);
	TTest::Printf(_L("Free space on drive %C = %ld (0x%lX)"), (TUint) aDrvCh, info.iFree, info.iFree);
	
	TInt buffsize;
	if (fast)
		{
		TInt64 bufSize64 = info.iFree / KNumBuf;
		buffsize = bufSize64 > KBufLen ? KBufLen : I64LOW(bufSize64);
		}
	else
		{
		buffsize = 100;
		}

	TTest::Printf(_L("Writing %d buffers of size %d"), KNumBuf, buffsize);


    TInt i;
    for (i = 0; i < KNumBuf; i++)
		gBuff[i].Fill('*', fast ? buffsize : 100);

    r = file.Replace(fs, name, EFileStreamText | EFileWrite);
    if (r != KErrNone)
        TTest::Fail(HERE, r, _L("opening %S for writing"), name.Ptr());

    TTime startTime;
    TTime endTime;
    TTimeIntervalMicroSeconds timeTaken;
    startTime.HomeTime();


    for (i = 0; i < KNumBuf; i++)
        {
        r = file.Write(gBuff[i]);
		if (r != KErrNone)
			{
			TTest::Printf(_L("Error writing file, r %d buffsize %d"), r, buffsize);
			file.Close();
			fs.Close();
			return r;
			}
        }

	file.Close();
	fs.Close();

    endTime.HomeTime();
    timeTaken=endTime.MicroSecondsFrom(startTime);

    TInt64 dtime = timeTaken.Int64();
	gWaitTime = I64LOW(dtime);

	if (gWaitTime < KTenthS)
		{
		TTest::Printf(_L("Time to complete writes (%d uS) too short"), gWaitTime);
		return KErrGeneral;
		}

	while (gWaitTime > 50*KSecond && buffsize > 100)
		{
		gWaitTime /= 10;
		buffsize /= 10;
		}

	for (i = 0; i < KNumBuf; i++)
		gBuff[i].Fill('*', buffsize);

	TTest::Printf(_L("Time to complete writes %d mS"), gWaitTime / 1000);

	return KErrNone;
	}

void PrintBufStatus()
	{
    for (TInt i = 0; i < KNumBuf; i++)
		{
		TTest::Printf(_L("Req %d r %08X\n"), i, gStat[i].Int());
		}
	}

LOCAL_C TInt TestClose(TChar aDrvCh, TBool aCloseFs)
///
/// Test what happens when a file is closed in the middle of writing it.  Note
/// that this assumes that the filesystem under test has been calibrated and
/// the buffers set up already.
///
/// @return KErrGeneral if an error was detected, KErrNone otherwise
///
    {
	TFileName fsname;
	TFileName name;
	RFile file;
	RFs   fs;
	TInt  drive;
	TInt  r;

	fs.Connect();

	r=TheFs.CharToDrive(aDrvCh, drive);
	TESTLIT(r, "CharToDrive", r==KErrNone);

	r=fs.FileSystemName(fsname, drive);
	TESTLIT(r, "FileSystemName", r==KErrNone);

	name.Format(_L("%C:\\cfstest.txt"), (TUint)aDrvCh);
	TTest::Printf(_L("Testing %S (%S)"), &name, &fsname);

    r = file.Replace(fs, name, EFileStreamText | EFileWrite);
    if (r != KErrNone)
        TTest::Fail(HERE, r, _L("opening %S for writing"), name.Ptr());

    TInt i;
    for (i = 0; i < KNumBuf; i++)
        {
        file.Write(gBuff[i], gStat[i]);
        }

	// wait for around a half of the writes to complete, then close the file
	// before the others finish.
	// If the media is unexpectedly slow and hasn't written a single buffer, wait a little bit longer
	TInt KMaxWaitTime =  Max(gWaitTime * 3, KSessionWaitSlow);
	TInt totalTimeWaited = 0;
	for (TInt waitTime = gWaitTime/2; 
		totalTimeWaited < KMaxWaitTime && gStat[0].Int() == KRequestPending;
		totalTimeWaited+= waitTime, waitTime = gWaitTime / 10)
		{
		TTest::Printf(_L("Waiting %d mS"), waitTime / 1000);
		User::After(waitTime);
		}

    TTest::Printf(_L("Wait ended"));
	if (aCloseFs)
		{
		TTest::Printf(_L("Closing file server session"));
		fs.Close();
		}
		
	else
		{
		TTest::Printf(_L("Closing file"));
		file.Close();
		}

    // test what has happened
	TInt nFinished  = 0;
	TInt nCancelled = 0;
	TInt nPending   = 0;
    TBool bad = EFalse;
	TRequestStatus tstat;
	RTimer timer;
	timer.CreateLocal();
	timer.After(tstat, 2 * gWaitTime);
	TInt requestsCompleted = 0;
    for (i = 0; i < KNumBuf; i++)
        {
        User::WaitForRequest(tstat, gStat[i]);
		++requestsCompleted;
		if (tstat != KRequestPending)
			{
			TTest::Printf(_L("Timer expired"));
			break;
			}
        r = gStat[i].Int();
        switch (r)
            {
            case KErrNone:
                TTest::Printf(_L("write %d finished\n"), i);
				nFinished++;
				if (nCancelled > 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nCancelled > 0"));
					}
                break;
            case KErrCancel:
                TTest::Printf(_L("write %d cancelled\n"), i);
				nCancelled++;
#if !defined(__WINS__) // under some loads the calibration is wrong by now and no writes have finished yet
				if (nFinished == 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nFinished == 0"));
					}
#endif
                break;
			case KRequestPending:
                TTest::Printf(_L("write %d pending\n"), i);
				nPending++;
				if (nCancelled > 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nCancelled > 0"));
					}
#if !defined(__WINS__) // timing issue as above
				if (nFinished == 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nFinished == 0"));
					}
#endif
                break;
            default:
				TTest::Fail(HERE, r, _L("incorrect status for write %d"), i);
            }
        }

	while (i < KNumBuf)
		{
        r = gStat[i].Int();
        switch (r)
            {
            case KErrNone:
                TTest::Printf(_L("write %d finished\n"), i);
				nFinished++;
				if (nCancelled > 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nCancelled > 0"));
					}
                break;
            case KErrCancel:
                TTest::Printf(_L("write %d cancelled\n"), i);
				nCancelled++;
#if !defined(__WINS__) // under some loads the calibration is wrong by now and no writes have finished yet
				if (nFinished == 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nFinished == 0"));
					}
#endif
                break;
			case KRequestPending:
                TTest::Printf(_L("write %d pending\n"), i);
				nPending++;
				if (nCancelled > 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nCancelled > 0"));
					}
#if !defined(__WINS__) // timing issue as above
				if (nFinished == 0)
					{
					bad = ETrue;
					PrintBufStatus();
					TTest::Fail(HERE, _L("nFinished == 0"));
					}
#endif
                break;
            default:
				TTest::Fail(HERE, r, _L("incorrect status for write %d"), i);
			}
		i++;
		}

	if (!aCloseFs)
		{
		// If we HAVEN'T closed the file server session, we must wait for all the remaining
		// requests, including the timer.
		
		TTest::Printf(_L("Waiting for %d uncompleted requests\n"), KNumBuf + 1 - requestsCompleted);
		for (i = requestsCompleted ; i < KNumBuf + 1 ; ++i)
			User::WaitForAnyRequest();
		}
	else
		{
		// If we have closed the session, then we need to work out if the timer went off (by
		// checking if there are any writes still pending) and if not then we need to wait for it.
		
		if (nPending == 0)
			{
			TTest::Printf(_L("Waiting for timer to complete\n"));
			User::WaitForRequest(tstat);
			}
		}

	if (aCloseFs)
		fs.Connect();

	TEntry entry;
	r = fs.Entry(name, entry);
	if (r != KErrNone)
		TTest::Fail(HERE, r, _L("Entry(%S)"), &name);

	TInt nWritten = entry.iSize / gBuff[0].Length();
	TTest::Printf(_L("Created file %d bytes, %d complete buffers written"),
				  entry.iSize, nWritten);
	
    fs.Delete(name);

	fs.Close();

	if (nCancelled < 2)
		{
		// At present, undefined whether cancelled or left pending
		// TTest::Fail(HERE, _L("Not enough buffers (%d) cancelled"), nCancelled);
		}

	if (nPending > 0)
		{
		// At present, undefined whether cancelled or left pending
		// TTest::Fail(HERE, _L("Too many buffers (%d) still pending"), nPending);
		}

	// Get the TFileCacheFlags for this drive
	TVolumeInfo volInfo;	// volume info for current drive
	r = TheFs.Volume(volInfo, drive);
	test(r==KErrNone);
	test.Printf(_L("DriveCacheFlags = %08X\n"), volInfo.iFileCacheFlags);

	// There's no point testing the numbers of buffers written if write caching enabled, as
	// in this case, the write requests are completed successfully before the data is written to disk
	if ((volInfo.iFileCacheFlags & EFileCacheWriteOn) && aCloseFs)
		{
		test.Printf(_L("%d write requests completed, %d of %d buffers written to disk\n"), nFinished, nWritten, KNumBuf);
		test.Printf(_L("Skipping test for buffers written as write caching enabled and FS session destroyed\n"));
		}
	else
		{

#if !defined(__WINS__) // timing issue as above
		if (nWritten < 2)
			{
			TTest::Fail(HERE, _L("Not enough buffers (%d) written"), nFinished);
			}
#endif
		if (nWritten < nFinished)
			{
			TTest::Fail(HERE, _L("Only %d buffers written, should be %d"), nWritten, nFinished);
			}
		}

    if (bad)
        TTest::Printf(_L("Test %c: FAILED\n"), name[0]);
    else
        TTest::Printf(_L("Test %c: OK\n"), name[0]);
	
    return (bad ? KErrGeneral : KErrNone);
    }

LOCAL_C TInt TestCloseOperation()
///
/// Task to exercise the remote (special) filesystem.
///
	{
	TSoakRemote rem(gDrvCh1);
	TInt r = KErrNone;
	TBool ok = ETrue;

	test.Next(_L("Mount as synchronous"));
	rem.Remount(ETrue);
	r = TestCalibrate(gDrvCh1);
	if (r == KErrNone)
		{
		test.Next(_L("Test SubSession synchronous"));
		r = TestClose(gDrvCh1, EFalse);
		if (r != KErrNone)
			ok = EFalse;
		test.Next(_L("Test Session synchronous"));
		r = TestClose(gDrvCh1, ETrue);
		if (r != KErrNone)
			ok = EFalse;
		}

	test.Next(_L("Mount as asynchronous"));
	rem.Remount(EFalse);
	r = TestCalibrate(gDrvCh1);
	if (r == KErrNone)
		{
		test.Next(_L("Test SubSession asynchronous"));
		r = TestClose(gDrvCh1, EFalse);
		if (r != KErrNone)
			ok = EFalse;
		test.Next(_L("Test Session asynchronous"));
		r = TestClose(gDrvCh1, ETrue);
		if (r != KErrNone)
			ok = EFalse;
		}

	return (ok ? KErrNone : KErrGeneral);
	}

LOCAL_C TInt parseCmd(TInt aArgC, TPtrC aArgV[], TChar& dr0, TChar& dr1, TChar& dr2)
///
/// Get drive characters from the command line.
///
    {
	if (aArgC > 1)
		dr0 = User::UpperCase(aArgV[1][0]);
	if (aArgC > 2)
		dr1 = User::UpperCase(aArgV[2][0]);
	if (aArgC > 3)
		dr2 = User::UpperCase(aArgV[3][0]);

    if (dr1 < 'A' || dr1 > 'Z')
		dr1 = dr0;

    return KErrNone;
    }

GLDEF_C void CallTestsL()
///
/// Do all tests.
///
	{
    //-- set up console output 
    F32_Test_Utils::SetConsole(test.Console()); 

	for (TInt i = 0; i < KMaxDrives; i++)
		{
		gPrimaryExtensions[i].iExists = EFalse;
		}


	// first thing, initialise local test class to make sure it gets done.
	TInt r = TTest::Init();
	test(r == KErrNone);
	
	const TInt KMaxArgs = 4;
	TPtrC argv[KMaxArgs];
	TInt  argc = TTest::ParseCommandArguments(argv, KMaxArgs);

	gDrvCh0 = TTest::DefaultDriveChar();

	r = parseCmd(argc, argv, gDrvCh0, gDrvCh1, gDrvCh2);
	if (r != KErrNone)
		{
		User::Exit(KErrAbort);
		}

	gRemFsChr = gDrvCh0;
	r = TheFs.CharToDrive(gRemFsChr, gRemFsDrv);

    test(r == KErrNone);
    
    PrintDrvInfo(TheFs, gRemFsDrv);

	if (r != KErrNone || gRemFsDrv == EDriveZ)
		{
		test.Printf(_L("Test cannnot run on drive %C:\n"), (TUint)gRemFsChr);
		return;
		}

	TDriveInfo drv;
	r = TheFs.Drive(drv, gRemFsDrv);
	test(r == KErrNone);

    if(Is_Automounter(TheFs, gRemFsDrv))
    {
		//-- this test tries to mount the curent drive as a synchronous one, 
        //-- automounter can't work on synchronous drives.
        test.Printf(_L("This test can't be performed on AutoMounter FS, Skipping.\n"));
		return;
    }

	// if it's a RAM drive or no media present, use the slow FS
	if (drv.iType == EMediaRam)
		{
		test.Printf(_L("Test can't run on RAM drive %C:\n"), (TUint)gRemFsChr);
		return;
		}
	if (drv.iType == EMediaUnknown || drv.iType == EMediaNotPresent)
		{
		// Mount test filesystem on that drive
		test.Printf(_L("Using slow filesystem on drive %C:\n"), (TUint)gRemFsChr);
		MountTestFileSystem(gRemFsDrv);
		}
	else if (argc > 2)
		{
		// two parameters, mount the test filesystem on the first
		MountTestFileSystem(gRemFsDrv);
		}
	else
		{
		// Use the existing filesystem on the drive
		gRemFsChr = 0;
		}

	// If we can do it, check whether the drive is LFFS and is OK
// !!! Disable platform security tests until we get the new APIs
//	if (User::Capability() & KCapabilityRoot)
//		CheckMountLFFS(TheFs, gDrvCh1);

    //!!!!! Warning: MountTestFileSystem() and UnmountFileSystem() code is lousy! it always mounts back the drive as SYNCHRONOUS.
    //!!!!! anyway, this code seems not to be called, because of "else if (argc > 2)"

	// Do tests
	r = TestCloseOperation();

	// restore the original mounted drive if anything changed
	UnmountFileSystem(gRemFsDrv);

	test(r == KErrNone);
	}


