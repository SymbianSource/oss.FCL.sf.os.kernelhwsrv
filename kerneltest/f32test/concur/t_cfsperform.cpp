// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// tests read/write throughput on two drives simultaneously
// 
//

//! @file f32test\concur\t_cfsbench.cpp

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_tdebug.h"

//IMPORT_C TUint32 DebugRegister();


GLDEF_D RTest test(_L("T_CFSBENCH"));
GLDEF_D	RFs TheFs;

LOCAL_D TFullName gFsName;
LOCAL_D TFullName gFsName1;
LOCAL_D TFullName gFsName2;
LOCAL_D TFullName gOldFsName;
LOCAL_D TFullName gNewFsName;
LOCAL_D TBool     gNoMedia = ETrue;

_LIT(KFsFile, "CFAFSDLY");
_LIT(KFsName, "DelayFS");

LOCAL_D const TInt32 KSecond = 1000000;
LOCAL_D const TInt32 KTimeBM = 20;


LOCAL_D const TInt32 KBufLen = 0x100;

LOCAL_D const TInt32 KMaxLag = 4;

LOCAL_D TBool gVerbose = EFalse;

TBuf16<KBufLen> gResults;

const TInt KMaxFileSize = (4*1024*1024);
const TInt KMinBufferSize = (16);
const TInt KMaxBufferSize = (512*1024);
const TInt KMaxIter = 17;


TBool gReadTests = EFalse;
TBool gWriteTests = EFalse;
TBool gAsyncTests = EFalse;
TBool gSyncTests = EFalse;


LOCAL_C TInt32 GetSpeed(TInt aOps, TInt aBufSize, TInt64 aDtime)
/// Calculate and return the throughput from the umber of blocks transferred
/// and the elapsed time.
	{
	TInt64 dsize = MAKE_TINT64(0, aOps) * MAKE_TINT64(0, aBufSize) * MAKE_TINT64(0, KSecond);
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
		if(r!=KErrNone)
			{
			test.Printf(_L("Error = %d"),r);
			test(EFalse);
			}
		}

	TBufC<16> type = _L("asynchronous");
	if (aSync)
		type = _L("synchronous");
	if (gVerbose)
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

class RFileOps
/// Do operations on a file.
	{
public:
	RFileOps();

	enum TOper
	{
	ERead,
	EWrite
	};
	
	
	TInt Init(TChar dr, TInt aBufSize);
	void DeInit();
	void CalculateFreeSpace();

	TInt Open(TOper aOper);
	TInt Close();
	TInt Delete();
	TInt Reset();
	TInt Erase();
	TInt Write();
	TInt Read();
	TInt End();
	TInt CreateReadFile();

public:
	TFileName      iNameRead;
	TFileName      iNameWrite;
	RFile          iF;

	HBufC8* iBuffer[KMaxLag];
	TPtr8* iBufPtr[KMaxLag];

	TRequestStatus iStatus[KMaxLag];
	TInt iPtr;
	TInt iNum;
	TInt iOps;
	TInt iMax;
	TBool iOpen;
	TInt iBufSize;
	TChar iDrvCh;
	TInt64 iFree;
	TInt iFileSize;
	// counters
	TInt iFileWraps;
	TInt iFileSyncAccesses;
	TInt iFileAsyncAccesses;
	};

RFileOps::RFileOps() : iPtr(0), iNum(0), iOps(0), iMax(0), iOpen(EFalse)
	{
	for (TInt i = 0; i < KMaxLag; i++)
		{
		iStatus[i] = KErrNone;
		iBuffer[i] = NULL;
		iBufPtr[i] = NULL;
		}

	}

TInt RFileOps::Init(TChar aDrvCh, TInt aBufSize)
	{
	TInt r = KErrNone;

	test(!iOpen);

	iDrvCh = aDrvCh;
	iBufSize = aBufSize;
	iNameWrite.Format(_L("%c:\\TESTWT"), (TUint)aDrvCh);
	iNameRead.Format(_L("%c:\\TESTRD"), (TUint)aDrvCh);
	
	for (TInt i = 0; i < KMaxLag; i++)
		{
		iStatus[i] = KErrNone;

		iBuffer[i] = HBufC8::NewL(aBufSize);
		if (iBuffer[i] == NULL)
			return KErrNoMemory;
		iBufPtr[i] = new TPtr8(iBuffer[i]->Des());
		//TPtr8 buffer(iBuffer[i]->Des());
		//buffer.Fill(TChar('_'), aBufSize);
		iBufPtr[i]->Fill(TChar('_'), aBufSize);
		}

	return r;
	}

void RFileOps::DeInit()
	{
	test(!iOpen);

	for (TInt i = 0; i < KMaxLag; i++)
		{
		delete iBuffer[i];
		iBuffer[i] = NULL;
		delete iBufPtr[i];
		iBufPtr[i] = NULL;
		}

	}

void RFileOps::CalculateFreeSpace()
	{
	TVolumeInfo vol;
	TInt        drv;
	
	
	TInt r = TheFs.CharToDrive(iDrvCh, drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("CharToDrive(%c) returned %d"), (TUint)iDrvCh, r);

	r = TheFs.Volume(vol, drv);
	if (r != KErrNone)
		TTest::Fail(HERE, _L("Volume(%c:) returned %d"), (TUint)iDrvCh, r);

	iFree = vol.iFree;

	TInt64 fileSize = iFree / 2;
	if (fileSize > KMaxFileSize)
		fileSize = KMaxFileSize;
	iFileSize = I64LOW(fileSize);


	// calculate the number of buffers to use
	// if we assume we'll be able to use half the available disk space
	TInt max = iFileSize / iBufSize;
	iMax = max;

    if (gVerbose)
		{
		test.Printf(_L("Free space on drive %c = %d KB\n"), (TUint)iDrvCh, I64LOW(iFree/1024));
		test.Printf(_L("File Size = %d KB. Using %d buffers of size %d\n"), iFileSize/1024, iMax, iBufSize);
		}

	}


TInt RFileOps::Open(TOper aOper)
/// Open the file for testing, give error if there is not enough space for it.
	{
	TInt r;

	test(!iOpen);


	// reset counters
	iFileWraps = 0;
	iFileSyncAccesses = 0;
	iFileAsyncAccesses = 0;
	
	TheFs.Delete(iNameWrite);

	if (aOper == ERead)
		{
		r = iF.Open(TheFs, iNameRead, EFileStreamText | EFileRead);
		if (r != KErrNone)
			return r;

		TInt sizeFile = 0;
		r = iF.Size(sizeFile);
		test_KErrNone(r);
        if (gVerbose)
			{
			test.Printf(_L("File Size = %d, %d buffers of size %d\n"), sizeFile, iMax, iBufSize);
			}
		iMax = sizeFile / iBufSize;
		}
	else
		{
		CalculateFreeSpace();

		if (iMax < KMaxLag)
			{
			test.Printf(_L("Insufficient free space on drive %c, deleting read file\n"), (TUint)iDrvCh);
			TInt maxOld = iMax;
			TheFs.Delete(iNameRead);
			CalculateFreeSpace();
			test.Printf(_L("Old available buffers = %d, new available buffers = %d\n"), maxOld, iMax);
			}
			if (iMax < KMaxLag)
				TTest::Fail(HERE, _L("Not enough space to do test, only %d KB available. Only %d buffers of %d bytes available"),
												  I64LOW(iFree/1024), iMax, iBufSize );

		r = iF.Replace(TheFs, iNameWrite, EFileStreamText | EFileWrite);
		}

	if (r == KErrNone)
		iOpen = ETrue;

	
	Reset();

	return r;
	}

// Close and delete the file, returning the number of operations done.
TInt RFileOps::Close()
	{
	if (!iOpen)
		return 0;

	iF.Close();
	iOpen = EFalse;

	// always delete the write file
	TheFs.Delete(iNameWrite);
	
	
	return iNum;
	}

TInt RFileOps::Delete()
	{
	TInt r = TheFs.Delete(iNameRead);
	r = TheFs.Delete(iNameWrite);
	return r;
	}


TInt RFileOps::Reset()
/// Reset all of the counts.
	{
	TInt err = KErrNone;

	iPtr = 0;
	iNum = 0;
	iOps = 0;
	return err;
	}

TInt RFileOps::CreateReadFile()
	{
	TInt r = KErrNone;

	CalculateFreeSpace();	// get iMax

	if (iOpen)
		iF.Close();
	iOpen = EFalse;


	r = iF.Open(TheFs, iNameRead, EFileStreamText | EFileRead);
	if (r == KErrNone)
		{
		if (gVerbose)
			test.Printf(_L("temp file already exists.\n"));
		iF.Close();

		return r;
		}


	r = iF.Replace(TheFs, iNameRead, EFileStreamText | EFileWrite);
	if (r != KErrNone)
		return r;
	
	iOpen = ETrue;
	
	Reset();


	test.Printf(_L("Creating temp file on drive %c of size %d..."), (TUint)iDrvCh, iFileSize);

	HBufC8* buf = HBufC8::NewL(KMaxBufferSize);
	TPtr8 bufptr(buf->Des());
	bufptr.Fill(TChar('_'), KMaxBufferSize);

	test(buf != NULL);
	for (TInt pos=0; pos<iFileSize; pos+= buf->Length())
		{
		r = iF.Write(pos, bufptr);
		test_KErrNone(r);
		}
	delete buf; buf = NULL;

	//	if (gVerbose)
		test.Printf(_L("Done.\n"));

	iF.Close();
	iOpen = EFalse;

	return r;
	}

TInt RFileOps::Write()
/// If there is a free buffer available, start a write.
	{
	if (!iOpen)
		return 0;

	while (iNum < iOps && iStatus[iNum%KMaxLag] != KRequestPending)
		{
		TInt status = iStatus[iNum%KMaxLag].Int();
		test (status == KErrNone);
		iNum++;
		}
	
	if (iOps < KMaxLag || iStatus[iPtr] != KRequestPending)
		{
		TInt pos = iOps%iMax * iBufSize;

		iF.Write(pos, *iBufPtr[iPtr], iStatus[iPtr]);

		TInt status = iStatus[iPtr].Int();

        if (gVerbose)
			{
			test.Printf(_L("Writing buf #%d to drive %c at pos %d, status = %d\n"), 
				iPtr, (TUint)iDrvCh, pos, status);
			}
		test (status == KErrNone || status == KRequestPending);

		iOps++;
		iPtr++;
		iPtr %= KMaxLag;
		return 1;
		}
	return 0;
	}

TInt RFileOps::Read()
/// If there is a free buffer available, start a read.
	{
	if (!iOpen)
		return 0;

	while (iNum < iOps && iStatus[iNum%KMaxLag] != KRequestPending)
		{
		TInt status = iStatus[iNum%KMaxLag].Int();
		if (status != KErrNone)
			test.Printf(_L("drive %c, iNum = %d, iOps=%d, Status = %d\n"), (TUint)iDrvCh, iNum, iOps, status);
		test (status == KErrNone);

		iNum++;
		}
	if (iOps < KMaxLag || iStatus[iPtr] != KRequestPending)
		{
		TInt pos = iOps%iMax * iBufSize;

		iBufPtr[iPtr]->SetLength(0);

		iF.Read(pos, *iBufPtr[iPtr], iStatus[iPtr]);

		TInt len = iBufPtr[iPtr]->Length();
		TInt status = iStatus[iPtr].Int();
		
		TInt err = KErrNone;

		if (status == KErrNone)
			{
			iFileSyncAccesses++;
			if (len < iBufPtr[iPtr]->MaxLength())
				err = KErrUnderflow;
			}
		else if (status == KRequestPending)
			{
			iFileAsyncAccesses++;
			}
		else
			{
			err = status;
			}

		if (gVerbose || err != KErrNone)
			{
			test.Printf(_L("Reading buf #%d, drive %c, pos %d, status %d, len %d, iNum %d, iOps %d, iMax %d\n"), 
				iPtr, (TUint)iDrvCh, pos, status, len, iNum, iOps, iMax);
			}

		test (err == KErrNone);

		iOps++;
		iPtr++;
		iPtr %= KMaxLag;

		// have we wrapped to postion zero ?
		if (iOps % iMax == 0)
			iFileWraps++;

		return 1;
		}
	return 0;
	}


TInt RFileOps::End()
/// Wait until all outstanding operations have ended, then return the number.
	{
	if (!iOpen)
		return 0;

	if (gVerbose)
		test.Printf(_L("Waiting for reads/writes to %c to complete, iNum=%d, iOps=%d...\n"), (TUint)iDrvCh, iNum, iOps);
	
	while (iNum < iOps)
		{
		if (iStatus[iNum%KMaxLag] == KRequestPending)
			{
			User::WaitForRequest(iStatus[iNum%KMaxLag]);
			}
		else
			{
			TInt status = iStatus[iNum%KMaxLag].Int();
			if (gVerbose || (status != KErrNone && status != KRequestPending))
				test.Printf(_L("Buf#%d: Status = %d\n"), iNum, status);
			test (status == KErrNone || status == KRequestPending);
			iNum++;
			}
		}

	return iNum;
	}

LOCAL_C TInt testAsyncAccess(
	TInt aDrive1,
	TInt aDrive2, 
	TInt aBufSize1, 
	TInt aBufSize2, 
	TBool aSync1, 
	TBool aSync2,
	TInt& aThroughput1,
	TInt& aThroughput2)
//
// Test one drive against the other.
//
    {
	TInt r;

	RFileOps f1;
	RFileOps f2;

	TChar dc1;
	TChar dc2;

	TInt   op1 = 0;
	TInt   op2 = 0;
	RTimer timer;
	TRequestStatus tstat;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt64 dtime;

	aThroughput1 = aThroughput2 = 0;

	if (aBufSize1 == 0 && aBufSize2 == 0)
		return KErrNone;

	timer.CreateLocal();


	r = TheFs.DriveToChar(aDrive1, dc1);
	test_KErrNone(r);
	r = TheFs.DriveToChar(aDrive2, dc2);
	test_KErrNone(r);

	// allocate buffers
	r = f1.Init(dc1, aBufSize1);
	test_KErrNone(r);
	r = f2.Init(dc2, aBufSize2);
	test_KErrNone(r);

	
	_LIT(KSync, " sync");
	_LIT(KAsync, "async");
	if (gVerbose)
		test.Printf(_L("%c: (%S) %d, %c: (%S) %d\n"), 
			(TUint)dc1, 
			aSync1?&KSync:&KAsync,
			aBufSize1, 
			(TUint)dc2, 
			aSync2?&KSync:&KAsync,
			aBufSize2);

	RemountFileSystem(aDrive1, aSync1);
	RemountFileSystem(aDrive2, aSync2);

	if (gReadTests)
		{
		//********************************************************************
		// read test
		//********************************************************************

		if (aBufSize1 > 0)
			{
			r = f1.CreateReadFile();
			test_KErrNone(r);
			}
		if (aBufSize2 > 0)
			{
			r = f2.CreateReadFile();
			test_KErrNone(r);
			}

		if (aBufSize1 > 0)
			r = f1.Open(RFileOps::ERead);
		test_KErrNone(r);
		
		if (aBufSize2 > 0)
			r = f2.Open(RFileOps::ERead);
		test_KErrNone(r);


		timer.After(tstat, KTimeBM * KSecond);

		startTime.HomeTime();

		while (tstat == KRequestPending)
			{
			TInt num = 0;
			if (aBufSize1 > 0)
				num = f1.Read();
			if (aBufSize2 > 0)
				num += f2.Read();
			if (num == 0)
				User::WaitForAnyRequest();
			}
		timer.Cancel();	

		if (aBufSize1 > 0)
			op1 = f1.End();
		if (aBufSize2 > 0)
			op2 = f2.End();

		endTime.HomeTime();
		timeTaken=endTime.MicroSecondsFrom(startTime);

		//********************************************************************
		// Read test end
		//********************************************************************
		}	// if (gReadTests)
		
	if (gWriteTests)
		{
		//********************************************************************
		// write test
		//********************************************************************
		if (aBufSize1 > 0)
			{
			r = f1.Open(RFileOps::EWrite);
			test_KErrNone(r);
			}
	
		if (aBufSize2 > 0)
			{
			r = f2.Open(RFileOps::EWrite);
			test_KErrNone(r);
			}

		timer.After(tstat, KTimeBM * KSecond);

		startTime.HomeTime();

		while (tstat == KRequestPending)
			{
			TInt num = 0;
			if (aBufSize1 > 0)
				num = f1.Write();
			if (aBufSize2 > 0)
				num += f2.Write();
			if (num == 0)
				User::WaitForAnyRequest();
			}
		timer.Cancel();

		if (aBufSize1 > 0)
			op1 = f1.End();
		if (aBufSize2 > 0)
			op2 = f2.End();


		endTime.HomeTime();
		timeTaken=endTime.MicroSecondsFrom(startTime);

		//********************************************************************
		// Write test end
		//********************************************************************

		}	// if gWriteTests

	dtime = timeTaken.Int64();

	aThroughput1 = GetSpeed(op1, aBufSize1, dtime);
	aThroughput2 = GetSpeed(op2, aBufSize2, dtime);

	test.Printf(_L("%c:,%c:,%10d,%10d,%10d,%10d\n"), 
		(TUint)dc1, (TUint)dc2, 
		aBufSize1,aBufSize2,
		aThroughput1,
		aThroughput2
		);

	
	if (gVerbose)
		{
		test.Printf(_L("%c: %d async reads, %d sync reads, wraps = %d\n"), 
			(TUint)dc1, f1.iFileAsyncAccesses, f1.iFileSyncAccesses, f1.iFileWraps);
		test.Printf(_L("%c: %d async reads, %d sync reads, wraps = %d\n"), 
			(TUint)dc2, f2.iFileAsyncAccesses, f2.iFileSyncAccesses, f2.iFileWraps);
		}


	f1.Close();
	f2.Close();


	timer.Close();

	f1.DeInit();
	f2.DeInit();

	return KErrNone;
    }

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


typedef TInt RESULTS[KMaxIter][KMaxIter]; 
LOCAL_C void PrintResults(RESULTS& aResults, TChar aDrvCh, TChar aDrvCh2)
	{
	TInt bufSize2;
	TInt drive1Index, drive2Index;

	test.Printf(_L("*** Throughput for drive %c ***\n"), (TUint)aDrvCh);

	test.Printf(_L("         BufferSize (%C:)....\n"), (TUint)aDrvCh2);
	gResults.Zero();
	for (bufSize2 = drive2Index = 0; bufSize2 <= KMaxBufferSize; bufSize2 = bufSize2 << 1, drive2Index++)
		{
		gResults.AppendFormat(_L("%10d,"), bufSize2);
		if (bufSize2 == 0)
			bufSize2 = KMinBufferSize >> 1;
		}
	test.Printf(_L("%S\n"), &gResults);
	
	for (drive1Index = 0; drive1Index < KMaxIter; drive1Index++)
		{
		gResults.Zero();
		for (drive2Index = 0; drive2Index < KMaxIter; drive2Index++)
			{
			gResults.AppendFormat(_L("%10d,"), aResults[drive1Index][drive2Index]);
			}
		test.Printf(_L("%S\n"), &gResults);
		}

	}


//
// Do all tests
//
GLDEF_C void CallTestsL()
	{
	TInt r = TTest::Init();
	test_KErrNone(r);

	TChar drvch1 = 0;
	TChar drvch2 = 0;
	TInt  drive1;
	TInt  drive2;

	const TInt KMaxArgs = 5;
	TPtrC argv[KMaxArgs];
	TInt  argc = TTest::ParseCommandArguments(argv, KMaxArgs);
	if (argc > 1)
		drvch1 = User::UpperCase(argv[1][0]);
	if (argc > 2)
		drvch2 = User::UpperCase(argv[2][0]);

	TBool testFs = EFalse;

	for (TInt n=3; n<argc; n++)
		{
		if (argc > 3)
			{
			if (argv[n].Compare(_L("verbose")) == 0)
				gVerbose = ETrue;
			if (argv[n].Compare(_L("read")) == 0)
				gReadTests = ETrue;
			if (argv[n].Compare(_L("write")) == 0)
				gWriteTests = ETrue;

			if (argv[n].Compare(_L("async")) == 0)
				gAsyncTests = ETrue;
			if (argv[n].Compare(_L("sync")) == 0)
				gSyncTests = ETrue;

			if (argv[n].Compare(_L("testfs")) == 0)
				testFs = ETrue;
			}
		}

	if ((!gReadTests && !gWriteTests) ||
		(!gAsyncTests && !gSyncTests))
		{
	    test.Printf(_L("T_CFSPERFORM - tests read/write throughput on two drives simultaneously\n"));
	    test.Printf(_L("Syntax : t_cfsperform <Drive1> <Drive2> [verbose] [testfs] read|write sync|async\n"));
	    test.Printf(_L("Where  : async = concurrent access, sync = non-concurrent access\n"));
	    test.Printf(_L("E.g.   : t_cfsperform c d read async\n"));
	    test.Printf(_L("Press any key"));
		test.Getch();	
		test.Printf(_L("\n"));
		return;
		}


	r = parseCmd(drvch1, drvch2);
	if (r != KErrNone)
		{
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

	if (testFs)
		{
		MountTestFileSystem(drive1);
		MountTestFileSystem(drive2);
		}


	TInt bufSize1;
	TInt bufSize2;

	// delete temp files before starting
	RFileOps f;
	f.Init(drvch1, 256);
	f.Delete();
	f.DeInit();
	f.Init(drvch2, 256);
	f.Delete();
	f.DeInit();


	TInt resultsDrive1[KMaxIter][KMaxIter];
	TInt resultsDrive2[KMaxIter][KMaxIter];
	TInt drive1Index;
	TInt drive2Index;

	test.Printf(_L("      BufSize(%c) BufSize(%c) ThruPut(%c) ThruPut(%c) \n"),
				(TUint)drvch1, (TUint)drvch2, (TUint)drvch1, (TUint)drvch2);

	for (bufSize1 = drive1Index = 0; bufSize1 <= KMaxBufferSize; bufSize1 = bufSize1 << 1, drive1Index++)
		{
		for (bufSize2 = drive2Index = 0; bufSize2 <= KMaxBufferSize; bufSize2 = bufSize2 << 1, drive2Index++)
			{

//			// !!! Disable platform security tests until we get the new APIs
//			if (User::Capability() & KCapabilityRoot)
//				{
//				CheckMountLFFS(TheFs, drvch1);
//				CheckMountLFFS(TheFs, drvch2);
//				}

			if (gVerbose)
				test.Printf(_L("Using drives %c: (%S) and %c: (%S)\n"),
							(TUint)drvch1, &gFsName1, (TUint)drvch2, &gFsName2);

			TInt throughputDrive1;
			TInt throughputDrive2;
			TBool sync = EFalse;
			if (gSyncTests)
				sync = ETrue;

			testAsyncAccess(
				drive1, drive2, 
				bufSize1, bufSize2, 
				sync, sync, 
				throughputDrive1, throughputDrive2);

			resultsDrive1[drive1Index][drive2Index] = throughputDrive1;
			resultsDrive2[drive1Index][drive2Index] = throughputDrive2;

			// buffer size sequence is 0,16,32,64,128, ...
			if (bufSize2 == 0)
				bufSize2 = KMinBufferSize >> 1;
			}
		// buffer size sequence is 0,16,32,64,128, ...
		if (bufSize1 == 0)
			bufSize1 = KMinBufferSize >> 1;
		}



	PrintResults(resultsDrive1, drvch1, drvch2);
	PrintResults(resultsDrive2, drvch2, drvch2);


	if (testFs)
		{
		UnmountFileSystem(drive1);
		UnmountFileSystem(drive2);
		}
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
