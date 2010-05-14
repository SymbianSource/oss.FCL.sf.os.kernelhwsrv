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
// Do benchmarking comparisons in asynchronous and synchronous modes.
//
//

//! @file f32test\concur\t_cfsbench.cpp

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_tdebug.h"

// The following #defines are for using older (and less accurate) benchmark
// timings.  They use multiple threads to get the operations simultaneous
// but this is inherrently inaccurate (it depends whether one of them starts
// and/or ends before the other how accurate the timings are).  If you leave
// them both commented out then the tests will be done in a single thread
// using asynchronous file operations, thus avoiding the problem.

// Uncomment the following if you want to test asynchronous file operations
// using two threads rather than in a single thread.

// #define TEST_ASYNC_IN_THREAD

// Uncomment the following if you want to test using synchronous file
// operations, using two threads to do both at once.

// #define TEST_SYNC_IN_THREAD

struct TStats
//
// Statistics -- size and time of operations.
//
	{
	TInt64 iSize;
	TInt64 iTime;
	void Init() { iSize = 0; iTime = 0; }
	};

GLDEF_D RTest test(_L("T_CFSBENCH"));
GLDEF_D	RFs TheFs;

LOCAL_D TFullName gFsName;
LOCAL_D TFullName gFsName1;
LOCAL_D TFullName gFsName2;
LOCAL_D TFullName gOldFsName;
LOCAL_D TFullName gNewFsName;
LOCAL_D TBool     gNoMedia = ETrue;

#if defined(TEST_SYNC_IN_THREAD) || defined(TEST_ASYNC_IN_THREAD)
LOCAL_D TInt      gThreadNumber = 0;
#endif

LOCAL_D RMutex gDataLock;
LOCAL_D TStats gWrStats;
LOCAL_D TStats gRdStats;

_LIT(KFsFile, "CFAFSDLY");
_LIT(KFsName, "DelayFS");

LOCAL_D const TInt32 KSecond = 1000000;
LOCAL_D const TInt32 KTimeBM = 20;
LOCAL_D const TInt32 KNumBuf = 100;
LOCAL_D const TInt32 KBufLen = 0x100;
LOCAL_D const TInt32 KMaxThr = 10;
LOCAL_D const TInt32 KMaxLag = 4;

LOCAL_D TBuf8<KBufLen> gBufferArr[KMaxThr][KNumBuf];
LOCAL_D TRequestStatus gStatusArr[KMaxThr][KNumBuf];

LOCAL_C void AddStats(TStats& aStats, TInt64 aSize, TInt64 aTime)
/// Add values to the statistics.
	{
	gDataLock.Wait();
	aStats.iSize += aSize;
	aStats.iTime += aTime;
	gDataLock.Signal();
	}

LOCAL_C TInt GetSpeed(TStats& aStats)
/// Calculate and return the data throughput from the statistics, rounded.
	{
	gDataLock.Wait();
	TInt speed = I64LOW((aStats.iSize + aStats.iTime/2) / aStats.iTime);
	gDataLock.Signal();
	return speed;
	}

LOCAL_C TInt32 GetSpeed(TInt aOps, TInt64 aDtime)
/// Calculate and return the throughput from the umber of blocks transferred
/// and the elapsed time.
	{
	TInt64 dsize = MAKE_TINT64(0, aOps) * MAKE_TINT64(0, KBufLen) * MAKE_TINT64(0, KSecond);
	TInt32 speed = I64LOW((dsize + aDtime/2) / aDtime);
	return speed;
	}

LOCAL_C TBool DriveIsOK(TChar c)
/// Test that a selected drive leter is OK to write files.
	{
	TInt r;
	TInt drv;
	r=TheFs.CharToDrive(c, drv);
	if (r != KErrNone)
		return EFalse;
	TDriveInfo info;
	r=TheFs.Drive(info,drv);
	test_KErrNone(r);
	return (info.iDriveAtt != 0 && !(info.iDriveAtt & KDriveAttRom));
	}

LOCAL_C TChar MountTestFileSystem(TInt aDrive)
//
// Mount a new CTestFileSystem on the drive under test
//
	{
	TInt r;
	TBuf<64> b;
	TChar c;
	r=TheFs.DriveToChar(aDrive,c);
	test_KErrNone(r);
	b.Format(_L("Mount test file system on %c:"),(TUint)c);
	test.Next(b);

	r=TheFs.AddFileSystem(KFsFile);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);

	r=TheFs.FileSystemName(gOldFsName,aDrive);
	test_Value(r, r == KErrNone || r==KErrNotFound);

	TDriveInfo drv;
	r = TheFs.Drive(drv, aDrive);
	test_KErrNone(r);

	gNoMedia = (drv.iType == EMediaUnknown || drv.iType == EMediaNotPresent);

	if (gOldFsName.Length() > 0)
		{
		TTest::Printf(_L("Dismount %C: %S"), (TUint)c, &gOldFsName);
		r=TheFs.DismountFileSystem(gOldFsName,aDrive);
		test_KErrNone(r);
		}

	r=TheFs.MountFileSystem(KFsName,aDrive);
	test_KErrNone(r);

	r=TheFs.FileSystemName(gNewFsName,aDrive);
	test_KErrNone(r);
	test(gNewFsName.CompareF(KFsName)==0);
	return c;
	}

LOCAL_C void UnmountFileSystem(TInt aDrive)
/// Unmount a test filesystem and mount the old one.
	{
	TChar c;
	TInt r=TheFs.DriveToChar(aDrive,c);
	test_KErrNone(r);
	r=TheFs.DismountFileSystem(gNewFsName,aDrive);
	test_KErrNone(r);
	// if there's no media present, don't try to mount it
	if (gNoMedia)
		{
		test.Printf(_L("No media on %C: so don't remount it"), (TUint)c);
		}
	else if (gOldFsName.Length() > 0)
		{
		test.Printf(_L("Mount    %C: %S"), (TUint)c, &gOldFsName);
		r=TheFs.MountFileSystem(gOldFsName,aDrive);
		test_KErrNone(r);
		}
	if (r != KErrNone)
		test.Printf(_L("Error %d remounting %S on %C\n"), r, &gOldFsName, (TUint)c);
	}

LOCAL_C void RemountFileSystem(TInt aDrive, TBool aSync)
/// Unmount and remount the file system on the specified drive in the
/// selected mode.
/// @param aDrive Drive number (EDriveC etc.).
/// @param aSync  Mount synchronous if true, asynchronous if not.
	{
	TChar c;
	TInt r=TheFs.DriveToChar(aDrive,c);
	r=TheFs.FileSystemName(gFsName, aDrive);
	test_Value(r, r == KErrNone || r==KErrNotFound);

	if (gFsName.Length() > 0)
		{
		r=TheFs.DismountFileSystem(gFsName, aDrive);
		test_KErrNone(r);
		}

	TBufC<16> type = _L("asynchronous");
	if (aSync)
		type = _L("synchronous");
	test.Printf(_L("Mount filesystem %c: %-8S as %S\n"), (TUint)c, &gFsName, &type);

#ifdef __CONCURRENT_FILE_ACCESS__
	r=TheFs.MountFileSystem(gFsName, aDrive, aSync);
#else
	r=TheFs.MountFileSystem(gFsName, aDrive);
#endif

	test_KErrNone(r);
	}

enum TOper
	{
	ERead,
	EWrite
	};

// ---------------------------------------------------------------------------

#if defined(TEST_SYNC_IN_THREAD)

LOCAL_C TInt testSyncAccess(TAny* aData)
///
/// Test read file handling.
///
/// @param aData pointer to the thread data area
    {
	TThreadData& data = *(TThreadData*)aData;
	TFileName fileName = data.iFile;
	TBool     dowrite  = (data.iData != NULL);

	RFs   myFs;
	TInt r = myFs.Connect();
	TEST(r==KErrNone);

	r = myFs.SetSessionPath(gSessionPath);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("SetSessionPath returned %d"), r);

	TVolumeInfo vol;
	TInt        drv;
	r = myFs.CharToDrive(fileName[0], drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("CharToDrive(%c) returned %d"), fileName[0], r);
	r = myFs.Volume(vol, drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("Volume() returned %d"), r);

	TInt maxwrite = TInt(vol.iFree / 2 - KBufLen);
	if (maxwrite < KBufLen*2)
		TTest::Fail(HERE, _L("Not enough space to do test, only %d KB available"),
					 TInt(vol.iFree/1024));

    RFile f;
	RTimer timer;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken;

	TBuf8<KBufLen> buff;
	TRequestStatus tstat;

	TInt wrnum = 0;
	TInt rdnum = 0;

	timer.CreateLocal();

	if (dowrite)
		{
		// write tests

		r = f.Replace(myFs, fileName, EFileStreamText | EFileWrite);
		TEST(r==KErrNone);

		// wait for both tasks to have a chance to complete opening the files
		User::After(1000);

		buff.Fill('_', KBufLen);

		timer.After(tstat, KTimeBM * KSecond);

		startTime.HomeTime();

		while (tstat == KRequestPending)
			{
			TInt pos = (wrnum * KBufLen) % maxwrite;
			r = f.Write(pos, buff);
			TEST(r==KErrNone);
			++wrnum;
			}

		endTime.HomeTime();
		timeTaken=endTime.MicroSecondsFrom(startTime);

		TInt64 dtime = timeTaken.Int64();
		TInt64 dsize = wrnum * KBufLen * TInt64(KSecond);
		TInt32 speed = TInt32((dsize + dtime/2) / dtime);
		AddStats(gWrStats, dsize, dtime);

		TTest::Printf(_L("%8d writes in %6d mS = %8d bytes per second\n"),
					  wrnum, TInt32(dtime)/1000, speed);

		timer.Cancel();
		f.Close();
		}
	else
		{
		// read tests

		r = f.Open(myFs, fileName, EFileStreamText);
		TEST(r==KErrNone);

		// wait for both tasks to have a chance to complete opening the files
		User::After(1000);

		timer.After(tstat, KTimeBM * KSecond);

		startTime.HomeTime();

		while (tstat == KRequestPending)
			{
			TInt pos = (rdnum * KBufLen) % maxwrite;
			r = f.Read(pos, buff, KBufLen);
			TEST(r==KErrNone);
			++rdnum;
			}

		endTime.HomeTime();
		timeTaken=endTime.MicroSecondsFrom(startTime);

		TInt64 dtime = timeTaken.Int64();
		TInt64 dsize = rdnum * KBufLen * TInt64(KSecond);
		TInt32 speed = TInt32((dsize + dtime/2) / dtime);
		AddStats(gRdStats, dsize, dtime);

		// wait to allow the dust to settle
		User::After(KSecond);

		TTest::Printf(_L("%8d reads  in %6d mS = %8d bytes per second\n"),
					  rdnum, TInt32(dtime)/1000, speed);

		timer.Cancel();
		timer.Close();
		f.Close();

		// delete file after reading it
		myFs.Delete(fileName);
		}

	myFs.Close();
	return r;
    }

#endif

// ---------------------------------------------------------------------------

#if defined(TEST_ASYNC_IN_THREAD)

LOCAL_C TInt testAsyncAccess(TAny* aData)
//
/// Test read file handling.
///
/// @param aData pointer to the thread data area
    {
	TThreadData& data = *(TThreadData*)aData;
	TFileName fileName = data.iFile;
	TBool     dowrite  = (data.iData != NULL);
	TBuf8<KBufLen>* buffer = gBufferArr[data.iNum];
	TRequestStatus* status = gStatusArr[data.iNum];

	RFs   myFs;
	TInt r = myFs.Connect();
	TEST(r==KErrNone);

	r = myFs.SetSessionPath(gSessionPath);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("SetSessionPath returned %d"), r);

	TVolumeInfo vol;
	TInt        drv;
	r = myFs.CharToDrive(fileName[0], drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("CharToDrive(%c) returned %d"), fileName[0], r);
	r = myFs.Volume(vol, drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("Volume() returned %d"), r);

	TInt64 maxwrite = vol.iFree / 2 - KBufLen;
	if (maxwrite < KBufLen*2)
		TTest::Fail(HERE, _L("Not enough space to do test, only %d KB available"),
					 TInt(vol.iFree/1024));

    RFile f;
	RTimer timer;
	TRequestStatus tstat;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken;

	TInt wrnum = 0;
	TInt rdnum = 0;
	TInt opnum = 0;
	TInt opfin = 0;
	TInt i;

	timer.CreateLocal();

	if (dowrite)
		{
		r = f.Replace(myFs, fileName, EFileStreamText | EFileWrite);
		TEST(r==KErrNone);

		// wait for both tasks to have a chance to complete opening the files
		User::After(1000);

		for (i = 0; i < KNumBuf; i++)
			buffer[i].Fill('_', KBufLen);

		timer.After(tstat, KTimeBM * KSecond);

		startTime.HomeTime();

		while (tstat == KRequestPending)
			{
			TInt pos = TInt((wrnum * KBufLen) % maxwrite);
			TInt bnum = opnum++ % KNumBuf;
			f.Write(pos, buffer[bnum], status[bnum]);
			if (opnum - opfin > KMaxLag)
				{
				while (status[opfin % KNumBuf] == KRequestPending)
					User::WaitForRequest(status[opfin % KNumBuf]);
				opfin++;
				}
			++wrnum;
			}

		while (opfin < opnum)
			{
			while (status[opfin % KNumBuf] == KRequestPending)
				User::WaitForRequest(status[opfin % KNumBuf]);
			opfin++;
			}

		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken=endTime.MicroSecondsFrom(startTime);

		TInt64 dtime = timeTaken.Int64();
		TInt64 dsize = wrnum * KBufLen * TInt64(KSecond);
		TInt32 speed = TInt32((dsize + dtime/2) / dtime);
		AddStats(gWrStats, dsize, dtime);

		TTest::Printf(_L("%8d writes in %6d mS = %8d bytes per second\n"),
					  wrnum, TInt32(dtime)/1000, speed);
		}
	else
		{
		r = f.Open(myFs, fileName, EFileStreamText);
		TEST(r==KErrNone);

		timer.After(tstat, KTimeBM * KSecond);

		startTime.HomeTime();

		while (tstat == KRequestPending)
			{
			TInt pos = TInt((rdnum * KBufLen) % maxwrite);
			TInt bnum = opnum++ % KNumBuf;
			f.Read(pos, buffer[bnum], status[bnum]);
			if (opnum - opfin > KMaxLag)
				{
				User::WaitForRequest(status[opfin++ % KNumBuf]);
				}
			++rdnum;
			}

		while (opfin < opnum)
			{
			if (status[opfin % KNumBuf] == KRequestPending)
				User::WaitForRequest(status[opfin % KNumBuf]);
			opfin++;
			}

		endTime.HomeTime();
		timeTaken=endTime.MicroSecondsFrom(startTime);
		TInt64 dtime = timeTaken.Int64();
		TInt64 dsize = rdnum * KBufLen * TInt64(KSecond);
		TInt32 speed = TInt32((dsize + dtime/2) / dtime);
		AddStats(gRdStats, dsize, dtime);

		// wait to allow the dust to settle
		User::After(KSecond);

		TTest::Printf(_L("%8d reads  in %6d mS = %8d bytes per second\n"),
					  rdnum, TInt32(dtime)/1000, speed);

		myFs.Delete(fileName);
		}

	timer.Cancel();
	timer.Close();
	f.Close();
	myFs.Close();
	return r;
    }

#endif

// ---------------------------------------------------------------------------

class TFileOps
/// Do operations on a file.
	{
public:
	TFileOps();
	TInt Open(TChar dr, TInt n);
	TInt Close();
	TInt Reset();
	TInt Erase();
	TInt Write();
	TInt Read();
	TInt End();
public:
	TFileName      iName;
	RFile          iF;
	TBuf8<KBufLen> iBuffer[KMaxLag];
	TRequestStatus iStatus[KMaxLag];
	TInt           iPtr;
	TInt           iNum;
	TInt           iOps;
	TInt           iMax;
	TBool          iOpen;
	};

TFileOps::TFileOps() : iPtr(0), iNum(0), iOps(0), iMax(0), iOpen(EFalse)
	{
	for (TInt i = 0; i < KMaxLag; i++)
		{
		iStatus[i] = KErrNone;
		iBuffer[i].Fill(TChar('_'), KBufLen);
		}
	}

TInt TFileOps::Open(TChar aDrvCh, TInt aNum)
/// Open the file for testing, give error if there is not enough space for it.
/// @param aDrvCh Drive letter.
/// @param aNum   File number suffix.
	{
	TVolumeInfo vol;
	TInt        drv;
	TInt r = TheFs.CharToDrive(aDrvCh, drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("CharToDrive(%c) returned %d"), (TUint)aDrvCh, r);
	r = TheFs.Volume(vol, drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("Volume(%c:) returned %d"), (TUint)aDrvCh, r);

	iMax = I64LOW(vol.iFree / MAKE_TINT64(0,KBufLen)) / 2 - 1;
	if (iMax < 10)
		TTest::Fail(HERE, _L("Not enough space to do test, only %d KB available"),
							                  I64LOW(vol.iFree/1024));

	Reset();
	iName.Format(_L("%c:\\TEST_%d"), (TUint)aDrvCh, aNum);
	r = iF.Replace(TheFs, iName, EFileStreamText | EFileWrite);
	if (r == KErrNone)
		iOpen = ETrue;
	return r;
	}

TInt TFileOps::Close()
/// Close and delete the file, returning the number of operations done.
	{
	if (!iOpen)
		return 0;
	iF.Close();
	TheFs.Delete(iName);
	iOpen = EFalse;
	return iNum;
	}

TInt TFileOps::Reset()
/// Reset all of the counts.
	{
	iPtr = 0;
	iNum = 0;
	iOps = 0;
	return 0;
	}

TInt TFileOps::Write()
/// If there is a free buffer available, start a write.
	{
	if (!iOpen)
		return 0;
	while (iNum < iOps && iStatus[iNum%KMaxLag] != KRequestPending)
		iNum++;
	if (iOps < KMaxLag || iStatus[iPtr] != KRequestPending)
		{
		TInt pos = iNum%iMax * KBufLen;
		iF.Write(pos, iBuffer[iPtr], iStatus[iPtr]);
		iOps++;
		iPtr++;
		iPtr %= KMaxLag;
		return 1;
		}
	return 0;
	}

TInt TFileOps::Read()
/// If there is a free buffer available, start a read.
	{
	if (!iOpen)
		return 0;
	while (iNum < iOps && iStatus[iNum%KMaxLag] != KRequestPending)
		iNum++;
	if (iOps < KMaxLag || iStatus[iPtr] != KRequestPending)
		{
		TInt pos = iNum%iMax * KBufLen;
		iF.Read(pos, iBuffer[iPtr], iStatus[iPtr]);
		iOps++;
		iPtr++;
		iPtr %= KMaxLag;
		return 1;
		}
	return 0;
	}

TInt TFileOps::End()
/// Wait until all outstanding operations have ended, then return the number.
	{
	if (!iOpen)
		return 0;
	while (iNum < iOps)
		{
		if (iStatus[iNum%KMaxLag] == KRequestPending)
			User::WaitForRequest(iStatus[iNum%KMaxLag]);
		else
			iNum++;
		}
	if (iOps < iMax)
		iMax = iOps;
	return iNum;
	}

LOCAL_C TInt testAsyncAccess(TChar dc1, TChar dc2)
//
// Test one drive against the other.
//
    {
	TFileOps f1;
	TFileOps f2;

	f1.Open(dc1, 1);
	if (dc1 != dc2)
		f2.Open(dc2, 2);

	TInt   op1 = 0;
	TInt   op2 = 0;
	RTimer timer;
	TRequestStatus tstat;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken;

	timer.CreateLocal();

	timer.After(tstat, KTimeBM * KSecond);

	startTime.HomeTime();

	while (tstat == KRequestPending)
		{
		TInt num = f1.Write();
		num += f2.Write();
		if (num == 0)
			User::WaitForAnyRequest();
		}

	op1 = f1.End();
	op2 = f2.End();

	endTime.HomeTime();
	timeTaken=endTime.MicroSecondsFrom(startTime);

	TInt64 dtime = timeTaken.Int64();

	TTest::Printf(_L("%c: %8d writes in %6d mS = %8d bytes per second\n"),
				  (TUint)dc1, op1, I64LOW(dtime)/1000, GetSpeed(op1, dtime));

	if (dc1 != dc2)
		TTest::Printf(_L("%c: %8d writes in %6d mS = %8d bytes per second\n"),
					  (TUint)dc2, op2, I64LOW(dtime)/1000, GetSpeed(op2, dtime));

	AddStats(gWrStats, MAKE_TINT64(0, op1 + op2) * MAKE_TINT64(0, KBufLen) * MAKE_TINT64(0, KSecond), dtime);

	// now the reads!

	f1.Reset();
	f2.Reset();

	timer.After(tstat, KTimeBM * KSecond);

	startTime.HomeTime();

	while (tstat == KRequestPending)
		{
		f1.Read();
		f2.Read();
		User::WaitForAnyRequest();
		}

	op1 = f1.End();
	op2 = f2.End();

	endTime.HomeTime();
	timeTaken=endTime.MicroSecondsFrom(startTime);

	dtime = timeTaken.Int64();

	TTest::Printf(_L("%c: %8d reads  in %6d mS = %8d bytes per second\n"),
				  (TUint)dc1, op1, I64LOW(dtime)/1000, GetSpeed(op1, dtime));

	if (dc1 != dc2)
		TTest::Printf(_L("%c: %8d reads  in %6d mS = %8d bytes per second\n"),
					  (TUint)dc2, op2, I64LOW(dtime)/1000, GetSpeed(op2, dtime));

	AddStats(gRdStats, MAKE_TINT64(0, op1 + op2) * MAKE_TINT64(0, KBufLen) * MAKE_TINT64(0, KSecond), dtime);

	test.Printf(_L("\n"));
	test.Printf(_L("average write throughput = %d bytes/sec\n"), GetSpeed(gWrStats));
	test.Printf(_L("average read  throughput = %d bytes/sec\n"), GetSpeed(gRdStats));
	test.Printf(_L("\n"));
	gWrStats.Init();
	gRdStats.Init();

	timer.Cancel();
	timer.Close();
	f1.Close();
	f2.Close();
	// delay for a second to allow the close to complete before dismounting.
	User::After(1000000);
	return KErrNone;
    }

#if defined(TEST_SYNC_IN_THREAD) || defined(TEST_ASYNC_IN_THREAD)

LOCAL_C TInt CreateThread(TThreadFunction aFunc, TChar c, TOper aOper)
/// Create a thread to do the appropriate operation on a drive.
	{
	TBuf<2>  drive(_L("?"));
	TBuf<64> name;
    drive[0] = TText(c);
	drive.UpperCase();
	TThreadData& d = TTest::Data(gThreadNumber);
	d.iFile.Format(_L("%S:\\TEST%d.FILE"), &drive, gThreadNumber);
	d.iData = (aOper == EWrite ? &aOper : NULL);
	name.Format(_L("Test_%S_%d"), &drive, gThreadNumber);
	TInt r = TTest::Create(gThreadNumber, aFunc, name);
	++gThreadNumber;
	return r;
	}

LOCAL_C TInt RunThreads(TThreadFunction aFunc, TChar aDrive1, TChar aDrive2, TOper aOper)
/// Run threads to test one drive against the other at the same time.
/// The thread will report any error and return it as a value, the program will
/// exit at a higher level after cleaning up.
	{
	TInt r;
	gThreadNumber = 0;
	if ((r = CreateThread(aFunc, aDrive1, aOper)) != KErrNone) return r;
	if ((r = CreateThread(aFunc, aDrive2, aOper)) != KErrNone) return r;
	TTest::Printf();
	r = TTest::Run();
	TTest::Printf();
	return r;
	}

LOCAL_C TInt testThreads(TThreadFunction aFunc, TChar c, TChar d)
/// Run threads testing read and write of the drives both ways round.
/// The thread will report any error and return it as a value, the program will
/// exit at a higher level after cleaning up.
	{
	TInt r;
	if ((r = RunThreads(aFunc, c, d, EWrite)) != KErrNone) return r;
	if ((r = RunThreads(aFunc, c, d, ERead))  != KErrNone) return r;
	if ((r = RunThreads(aFunc, d, c, EWrite)) != KErrNone) return r;
	if ((r = RunThreads(aFunc, d, c, ERead))  != KErrNone) return r;
	// display totals;
	test.Printf(_L("average write throughput = %d bytes/sec\n"), GetSpeed(gWrStats));
	test.Printf(_L("average read  throughput = %d bytes/sec\n"), GetSpeed(gRdStats));
	test.Printf(_L("\n"));
	gWrStats.Init();
	gRdStats.Init();
	return r;
	}

#endif

LOCAL_C TInt parseCmd(TChar& aDrvCh1, TChar& aDrvCh2)
/// Get parameters from the comand line; if there aren't enough then
/// prompt the user for them and return KErrAbort if ^C is pressed.
	{
	while (aDrvCh1 < 'A' || aDrvCh1 > 'Z')
		{
		test.Printf(_L("Enter drive letter: "));
		while (aDrvCh1 < 'A' || aDrvCh1 > 'Z')
			{
			if (aDrvCh1 == 0x03)
				return KErrAbort;
			aDrvCh1 = User::UpperCase(test.Getch());
			}
		if (!DriveIsOK(aDrvCh1))
			{
			test.Printf(_L("%c: is not a valid drive\n"), (TUint)aDrvCh1);
			aDrvCh1 = 0;
			}
		else
			{
			TInt drv;
			TheFs.CharToDrive(aDrvCh1, drv);
			TheFs.FileSystemName(gFsName1, drv);
			test.Printf(_L("%c: (%S)\n"), (TUint)aDrvCh1, &gFsName1);
			}
		}

	while (aDrvCh2 < 'A' || aDrvCh2 > 'Z')
		{
		test.Printf(_L("Enter drive letter: "));
		while (aDrvCh2 < 'A' || aDrvCh2 > 'Z')
			{
			if (aDrvCh2 == 0x03)
				return KErrAbort;
			aDrvCh2 = User::UpperCase(test.Getch());
			}
		if (!DriveIsOK(aDrvCh2))
			{
			test.Printf(_L("%c: is not a valid drive\n"), (TUint)aDrvCh2);
			aDrvCh2 = 0;
			}
		else
			{
			TInt drv;
			TheFs.CharToDrive(aDrvCh2, drv);
			TheFs.FileSystemName(gFsName2, drv);
			test.Printf(_L("%c: (%S)\n"), (TUint)aDrvCh2, &gFsName2);
			}
		}
	return KErrNone;
	}

GLDEF_C void CallTestsL()
//
// Do all tests
//
	{
	TInt r = TTest::Init();
	test_KErrNone(r);

	TChar drvch0 = TTest::DefaultDriveChar();
	TChar drvch1 = 0;
	TChar drvch2 = 0;
	TInt  drive0;
	TInt  drive1;
	TInt  drive2;

	const TInt KMaxArgs = 4;
	TPtrC argv[KMaxArgs];
	TInt  argc = TTest::ParseCommandArguments(argv, KMaxArgs);
	if (argc > 1)
		drvch0 = User::UpperCase(argv[1][0]);
	if (argc > 2)
		drvch1 = User::UpperCase(argv[2][0]);
	if (argc > 3)
		drvch2 = User::UpperCase(argv[3][0]);

	r = TheFs.CharToDrive(drvch0, drive0);
	test_KErrNone(r);

	if (TheFs.IsValidDrive(drive0))
		MountTestFileSystem(drive0);
	else
		test.Printf(_L("Unable to mount test file system\n"));

	r = parseCmd(drvch1, drvch2);
	if (r != KErrNone)
		{
		UnmountFileSystem(drive0);
		User::Panic(_L("USER ABORT"), 0);
		}

	r = TheFs.CharToDrive(drvch1, drive1);
	test_KErrNone(r);
	r = TheFs.CharToDrive(drvch2, drive2);
	test_KErrNone(r);

	r = TheFs.FileSystemName(gFsName1, drive1);
	test_Value(r, r == KErrNone || r == KErrNotFound);
	r = TheFs.FileSystemName(gFsName2, drive2);
	test_Value(r, r == KErrNone || r == KErrNotFound);

	gDataLock.CreateLocal();

	if (drive1 == drive2)
		{
// !!! Disable platform security tests until we get the new APIs
//		if (User::Capability() & KCapabilityRoot)
//			CheckMountLFFS(TheFs, drvch1);

		test.Printf(_L("Using drive %c: (%S)\n"),
					(TUint)drvch1, &gFsName1);
		if (r == KErrNone)
			{
			test.Next(_L("Test with drive asynchronous"));
			RemountFileSystem(drive1, EFalse);
			testAsyncAccess(drvch1, drvch1);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test with drive synchronous"));
			RemountFileSystem(drive1, ETrue);
			testAsyncAccess(drvch1, drvch1);
			}
		}
	else
		{
// !!! Disable platform security tests until we get the new APIs
/*		if (User::Capability() & KCapabilityRoot)
			{
			CheckMountLFFS(TheFs, drvch1);
			CheckMountLFFS(TheFs, drvch2);
			}
*/
		test.Printf(_L("Using drives %c: (%S) and %c: (%S)\n"),
					(TUint)drvch1, &gFsName1, (TUint)drvch2, &gFsName2);

#if !defined(TEST_ASYNC_IN_THREAD)

		if (r == KErrNone)
			{
			test.Next(_L("Test async r/w with both drives async"));
			RemountFileSystem(drive1, EFalse);
			RemountFileSystem(drive2, EFalse);
			testAsyncAccess(drvch1, drvch2);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test async r/w with 1st drive sync and 2nd async"));
			RemountFileSystem(drive1, ETrue);
			RemountFileSystem(drive2, EFalse);
			testAsyncAccess(drvch1, drvch2);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test async r/w with 1st drive async and 2nd sync"));
			RemountFileSystem(drive1, EFalse);
			RemountFileSystem(drive2, ETrue);
			testAsyncAccess(drvch1, drvch2);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test async r/w with both drives sync"));
			RemountFileSystem(drive1, ETrue);
			RemountFileSystem(drive2, ETrue);
			testAsyncAccess(drvch1, drvch2);
			}

#else

		if (r == KErrNone)
			{
			test.Next(_L("Test async r/w with both drives asynchronous"));
			RemountFileSystem(drive1, EFalse);
			RemountFileSystem(drive2, EFalse);
			r = testThreads(testAsyncAccess, drvch1, drvch2);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test async r/w with one drive sync and one async"));
			RemountFileSystem(drive1, ETrue);
			RemountFileSystem(drive2, EFalse);
			r = testThreads(testAsyncAccess, drvch1, drvch2);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test async r/w with both drives synchronous"));
			RemountFileSystem(drive1, ETrue);
			RemountFileSystem(drive2, ETrue);
			r = testThreads(testAsyncAccess, drvch1, drvch2);
			}
#endif

#if defined(TEST_SYNC_IN_THREAD)

		if (r == KErrNone)
			{
			test.Next(_L("Test sync r/w with both drives asynchronous"));
			RemountFileSystem(drive1, EFalse);
			RemountFileSystem(drive2, EFalse);
			r = testThreads(testSyncAccess, drvch1, drvch2);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test sync r/w with one drive sync and one async"));
			RemountFileSystem(drive1, ETrue);
			RemountFileSystem(drive2, EFalse);
			r = testThreads(testSyncAccess, drvch1, drvch2);
			}

		if (r == KErrNone)
			{
			test.Next(_L("Test sync r/w with both drives synchronous"));
			RemountFileSystem(drive1, ETrue);
			RemountFileSystem(drive2, ETrue);
			r = testThreads(testSyncAccess, drvch1, drvch2);
			}
#endif
		}

	gDataLock.Close();

	UnmountFileSystem(drive0);
	test_Value(r, r == 0);
	}


GLDEF_C TInt E32Main()
//
// Main entry point
//
    {
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

	// Do the tests
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
