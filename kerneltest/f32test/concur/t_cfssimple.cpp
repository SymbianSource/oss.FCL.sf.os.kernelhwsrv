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
// f32test\concur\t_cfssimple.cpp
// 
//
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include "t_tdebug.h"

GLDEF_D RTest test(_L("T_CFSSIMPLE"));
GLDEF_D	RFs TheFs;

LOCAL_D TFullName OldFsName;
LOCAL_D TFileName NewFsName;

_LIT(KFsFile, "CFAFSDLY");
_LIT(KFsName, "DelayFS");


struct TExtension
	{
	TFullName iName;
	TBool iExists;
	};

LOCAL_D TExtension gPrimaryExtensions[KMaxDrives];

LOCAL_C TChar MountTestFileSystem(TInt aDrive)
//
// Mount a new test filesystem on the drive under test
//
	{
	TBuf<64> b;
	TChar c;
	TInt r;
	if (aDrive > EDriveZ)
		{
		c = aDrive;
		r=TheFs.CharToDrive(c, aDrive);
		test_KErrNone(r);
		}
	else
		{
		r=TheFs.DriveToChar(aDrive,c);
		test_KErrNone(r);
		}

	b.Format(_L("Test mounting of test file system on %c:"),(TUint)c);
	test.Next(b);

	test.Printf(_L("Load filesystem from %S\n"), &KFsFile);

	r=TheFs.AddFileSystem(KFsFile);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	
	r=TheFs.FileSystemName(OldFsName,aDrive);
	test_Value(r, r == KErrNone || r == KErrNotFound);

	if (OldFsName.Length() > 0)
		{
		r = TheFs.ExtensionName(gPrimaryExtensions[aDrive].iName, aDrive, 0);
		if (r == KErrNone)
			gPrimaryExtensions[aDrive].iExists = ETrue;

		test.Printf(_L("Dismount %S filesystem on %c:\n"), &OldFsName, (TUint)c);
		r=TheFs.DismountFileSystem(OldFsName,aDrive);
		if (r == KErrNotReady)
			{
			TDriveInfo d;
			r=TheFs.Drive(d, aDrive);
			test_KErrNone(r);
			if (d.iType == EMediaNotPresent)
				test.Printf(_L("%c: Medium not present - cannot perform test.\n"), (TUint)c);
			else
				test.Printf(_L("medium found (type %d) but drive %c: not ready\nPrevious test may have hung; else, check hardware.\n"), (TInt)d.iType, (TUint)c);
			}
		else if (r == KErrCorrupt)
			test.Printf(_L("drive %c: corrupted, cannot unmount\nPrevious test may have aborted; else, check hardware.\n"), (TUint)c);

		test_KErrNone(r);
		}
	
	test.Printf(_L("Mount filesystem %S\n"), &KFsName);

	if (gPrimaryExtensions[aDrive].iExists == EFalse)
		r=TheFs.MountFileSystem(KFsName,aDrive);
	else
		r=TheFs.MountFileSystem(KFsName,gPrimaryExtensions[aDrive].iName,aDrive);
	test_KErrNone(r);

	r=TheFs.FileSystemName(NewFsName,aDrive);
	test_KErrNone(r);
	test(NewFsName.Compare(KFsName)==0);
	test.Printf(_L("New %S filesystem on %c:\n"), &NewFsName, (TUint)c);

	// Check attributes
	TDriveInfo info;
	r=TheFs.Drive(info,aDrive);
	test_KErrNone(r);
 
	test.Printf(_L("iType=%d,iConnctionBusType=%d,iDriveAtt=%x,iMediaAtt=%x\n"),(TUint)info.iType,\
		(TUint)info.iConnectionBusType,info.iDriveAtt,info.iMediaAtt);
	return c;
	}

LOCAL_C void UnmountFileSystem(TInt aDrive)
//
// Unmount the test filesystem on the drive under test
//
	{
	TChar c;
	TInt r;
	if (aDrive > EDriveZ)
		{
		c = aDrive;
		r=TheFs.CharToDrive(c, aDrive);
		test_KErrNone(r);
		}
	else
		{
		r=TheFs.DriveToChar(aDrive,c);
		test_KErrNone(r);
		}
    test.Printf(_L("Dismount %S filesystem on %c\n"), &NewFsName, (TUint)c);
	r=TheFs.DismountFileSystem(NewFsName,aDrive);
	test_KErrNone(r);
	if (OldFsName.Length() > 0)
		{
		test.Printf(_L("Remount %S filesystem on %c\n"), &OldFsName, (TUint)c);

		if (gPrimaryExtensions[aDrive].iExists == EFalse)
			r=TheFs.MountFileSystem(OldFsName,aDrive);
		else
			r=TheFs.MountFileSystem(OldFsName,gPrimaryExtensions[aDrive].iName,aDrive);
		test_KErrNone(r);
		}
	}

template <TInt N>
LOCAL_C TDes* cvtbuff(const TDesC8& buff, TBuf<N>& b)
//
// Copy an 8-bit buffer to a character one.
//
	{
	TInt i;
	TInt len = buff.Length();
	TInt max = b.MaxLength();
	if (len > max)
		len = max;
	b.SetLength(len);
	for (i = 0; i < len; i++)
		b[i] = TText(buff[i]);
	return &b;
	}

//---------------------------------------------
//! @SYMTestCaseID FSBASE-CR-PCHY-6PALT2
//! @SYMTestType CT
//! @SYMREQ CR PCHY-6PALT2
//! @SYMTestCaseDesc Test asynchronous RFs::Flush(TRequestStatus& aStatus)
//! @SYMTestStatus Implemented
//! @SYMTestActions Perform an asynchronous flush and wait for completion
//! @SYMTestExpectedResults Asynchronous flush completes after expected delay
//! @SYMTestPriority Low
//! @SYMAuthor Stefan Williams
//! @SYMCreationDate 01/06/2006
//! @See EFSRV and EFILE components
//! @file f32test\concur\t_cfssimple.cpp
//---------------------------------------------

LOCAL_C void testAsyncFlush(RFile& aFile)
    {
 	TTest::Printf(_L("Test Async RFile::Flush()"));

	RTimer flushTimer;
	TRequestStatus flushTimerStatus;
	TInt err = flushTimer.CreateLocal();
    TEST(err == KErrNone);

	flushTimer.After(flushTimerStatus, 10000000); // 10 seconds timeout for RFile::Flush on CFAFSDLY file system

	TTime startFlush;
	startFlush.HomeTime();

	TRequestStatus flushStatus;
	aFile.Flush(flushStatus);
    TEST(flushStatus==KRequestPending);

	User::WaitForRequest(flushTimerStatus, flushStatus);
	if ((flushStatus == KRequestPending) && (flushTimerStatus == KErrNone))
		{
		TTest::Fail(HERE, _L("Flush : Timed Out"));
		}

	if (flushStatus != KErrNone)
		{
		TTest::Fail(HERE, flushStatus.Int(), _L("Flush : Error"));
		}

	TTime endFlush;
	endFlush.HomeTime();

    TTimeIntervalSeconds flushTime;
    err = endFlush.SecondsFrom(startFlush, flushTime);
    TEST(err == KErrNone);

	TTest::Printf(_L("Flush took %d seconds"), flushTime.Int());

	if(flushTime.Int() < 3)
		{
		TTest::Fail(HERE, _L("Flush took < 3 Seconds"));
		}

	flushTimer.Cancel();
	flushTimer.Close();
	}

LOCAL_C TInt testAccessFile(TAny*)
//
// Test read file handling with two threads.
//
    {
	TTest::Start(_L("Test write file"));
	
	TThreadData& data = TTest::Self();
	TFileName fileName = data.iFile;

	TTest::Printf(_L("Connect to myFs\n"));

	RFs   myFs;
	TInt r = myFs.Connect();
	TEST(r==KErrNone);

	TTest::Printf(_L("Create %S\n"), &fileName);

    RFile f;

    r = f.Replace(myFs, fileName, EFileStreamText | EFileWrite);
    TEST(r==KErrNone);

	// wait for both tasks to have a chance to complete opening the files
	User::After(1000);

	const TInt KNumBuf = 10;
	const TInt KBufLen = 0x80;
    TBuf8<KBufLen> a[KNumBuf];
    TBuf8<KBufLen> b;
	TBuf<KBufLen>  tbuf;
	TInt i;

	TUint base = 0;
	for (i = 0; i < fileName.Length(); i++)
		base = base * 0x11 + fileName[i];
	base += base / 32;
	base = base % 32 + 0x30;

	TRequestStatus status[10];

	TTest::Next(_L("Write file"));
    for (i = 0; i <= 9; i++)
        {
		TChar val = TChar(base + i);
		a[i].Fill(TText(val), i+1);
		TTest::Printf(_L("write '%S' started\n"), cvtbuff(a[i], tbuf));
        f.Write(a[i], status[i]);
		User::After(1);
		if (r != KErrNone)
			{
			TTest::Fail(HERE, r, _L("Write"));
			}
        }

	for (i = 0; i < 10; i++)
		{
		User::WaitForAnyRequest();
		if (status[i] == KRequestPending)
			{
			TTest::Printf(_L("write %d still pending!\n"), i);
			}
		else
			{
			TTest::Printf(_L("write %d completed with status %d\n"), i, status[i].Int());
			}
		}

	TTest::Printf(_L("Write 64 bytes at the end as padding\n"));
	TBuf8<32> fill;
	fill.Fill(TText('_'), 32);
	f.Write(fill);
	f.Write(fill);

	testAsyncFlush(f);

    f.Close();

	b.Fill(TText('*'), b.MaxLength());
	TTest::Next(_L("Read file"));

	// wait to allow the dust to settle
	User::After(1000000);
	
    r = f.Open(myFs, fileName, EFileStreamText);
    TEST(r==KErrNone);

	for (i = 0; i < 10; i++)
        {
         r=f.Read(b, 12-i);
		if (r == KErrNone)
			{
			TTest::Printf(_L("read %2d bytes <%S>\n"), b.Length(), cvtbuff(b, tbuf));
			}
		else
			{
			TTest::Fail(HERE, r, _L("Read"));
			}
		User::After(1);
        }
    r=f.Read(b);
	if (r != KErrNone)
		{
		TTest::Fail(HERE, r, _L("Read\n"));
		// TEST(r==KErrNone);
		}
    if (!(b.Length()==0))
		{
		cvtbuff(b, tbuf);
		TTest::Printf(_L("read %2d bytes <%.32S>\n"), b.Length(), &tbuf);
		}

	// wait to allow the dust to settle
	User::After(1000000);
	
	TTest::Printf(_L("Close file"));
    f.Close();

	TTest::Printf(_L("Close session"));
	myFs.Close();
	return r;
    }

LOCAL_C TInt createThread(TThreadFunction aFunc, TChar aDrvCh)
//
// Create a thread using function aFunc on drive aDrvCh
//
	{
	static TInt tnum = 0;
	TBuf<2>  drive(_L("?"));
	TBuf<64> name;
    drive[0] = TText(aDrvCh);
	drive.UpperCase();
	TThreadData& d = TTest::Data(tnum);
	d.iFile.Format(_L("%S:\\TEST%d.FILE"), &drive, tnum);
	name.Format(_L("Test_%S_%d"), &drive, tnum);
	TInt r = TTest::Create(tnum, aFunc, name);
	++tnum;
	return r;
	}

LOCAL_C TInt testThreads(TChar aDrvCh1, TChar aDrvCh2)
//
// Create threads operating on files on two drives.
//
	{
	TInt r;
	r = createThread(testAccessFile, aDrvCh1);
	r = createThread(testAccessFile, aDrvCh2);
	r = TTest::Run();
	return r;
	}

LOCAL_C TBool isSpace(TChar c)
//
// Return true if the character is space or tab
//
	{
	return (c == ' ' || c == '\t');
	}

LOCAL_C void parseCmd(TDesC& cmd, TChar& dr0, TChar& dr1, TChar& dr2)
// 
// Parse command line to get two characters for drive 1 and 2
//
	{
	TInt i = 0;
	TInt n = cmd.Length();
	// skip to first non-space
	while (i < n && isSpace(cmd[i]))
		++i;
	if (i < n)
		dr0 = cmd[i];
	// skip to first space
	while (i < n && !isSpace(cmd[i]))
		++i;
	// skip to first non-space
	while (i < n && isSpace(cmd[i]))
		++i;
	if (i < n)
		dr1 = cmd[i];
	// skip to first space
	while (i < n && !isSpace(cmd[i]))
		++i;
	// skip to first non-space
	while (i < n && isSpace(cmd[i]))
		++i;
	if (i < n)
		dr2 = cmd[i];
	// skip to first space
	while (i < n && !isSpace(cmd[i]))
		++i;
	}

GLDEF_C void CallTestsL()
//
// Do all tests
//
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	cmd.UpperCase();

	TChar dr0 = 'F';
	TChar dr1 = 0;
	TChar dr2 = 0;
	parseCmd(cmd, dr0, dr1, dr2);

	if (dr1 < 'A')
		dr1 = dr0;
	
	if (dr2 < 'A')
		dr2 = dr1;
	
	if (dr0 < 'A' || dr0 >= 'Z')
		{
		test.Printf(_L("Test cannnot run on drive\n"));
		return;
		}

	MountTestFileSystem(dr0);
	// TheFs.SetVolumeLabel(_L("TRACE"));
	TInt r = testThreads(dr1, dr2);
	TheFs.SetVolumeLabel(_L("OFF"));
	UnmountFileSystem(dr0);
	test_KErrNone(r);
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
    test_KErrNone(r);

    // TheFs.SetAllocFailure(gAllocFailOn);
    TTime timerC;
    timerC.HomeTime();

    TRAP(r,CallTestsL());

    // reset the debug register
    TheFs.SetDebugRegister(0);

    TTime endTimeC;
    endTimeC.HomeTime();
    TTimeIntervalSeconds timeTakenC;
    r=endTimeC.SecondsFrom(timerC,timeTakenC);
    test_KErrNone(r);
    test.Printf(_L("Time taken for test = %d seconds\n"),timeTakenC.Int());
    // TheFs.SetAllocFailure(gAllocFailOff);
    TheFs.Close();
    test.End();
    test.Close();
    __UHEAP_MARKEND;
    delete cleanup;
    return(KErrNone);
    }

