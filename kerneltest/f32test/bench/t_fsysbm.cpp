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
// f32test/bench/t_fsysbm.cpp
//
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <hal.h>
#include <e32math.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include "t_server.h"
#include "../../e32test/mmu/d_sharedchunk.h"

#define SYMBIAN_TEST_EXTENDED_BUFFER_SIZES	// test using a greater number of buffer sizes
//#define SYMBIAN_TEST_COPY					// read from one drive and write to another


RTest test(_L("File System Benchmarks"));

static const TUint K1K = 1024;								// 1K
static const TUint K1M = 1024 * 1024;						// 1M
static const TUint K2M = 2 * K1M;						    // 2M

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
const TInt64 KGb  	= 1 << 30;								// 1GB
const TInt64 K3GB   = 3 * KGb;								// 3GB
const TInt64 K4GB   = 4 * KGb;								// 4GB
#endif //SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

#if defined(__WINS__)
static TInt KMaxFileSize = 256 * K1K;					// 256K
//static TInt KMaxFileSize = K1M;						// 1M
#else
//static TInt KMaxFileSize = 256 * K1K;					// 256K
//static TInt KMaxFileSize = K1M;						// 1M
static TInt KMaxFileSize = K2M;							// 2M
#endif

const TTimeIntervalMicroSeconds32 KFloatingPointTestTime = 10000000;	// 10 seconds
static const TInt KHeapSize = 0x4000;

static TPtr8 DataBuf(NULL, KMaxFileSize,KMaxFileSize);
static HBufC8* DataBufH = NULL;

static RSharedChunkLdd Ldd;
static RChunk TheChunk;
static TInt PageSize;
const TUint ChunkSize = KMaxFileSize;


static RFile File, File2;
#if defined SYMBIAN_TEST_COPY
static TChar gDriveToTest2;
#endif

// if enabled, Read and Write operations are not boundary aligned.
static TBool gMisalignedReadWrites = EFalse;

// read & write caching enabled flags - may be overridden by +/-r +/-w command line switches
static TBool gReadCachingOn  = EFalse;
static TBool gWriteCachingOn = EFalse;

// if enabled, timings are for write AND flush
static TBool gFlushAfterWrite = ETrue;

// if enabled, contiguous shared memory is used for Data buffer
static TBool gSharedMemory = EFalse;

// if enabled, fragmented shared memory is used for Data buffer
static TBool gFragSharedMemory = EFalse;

// if enabled, file is opened in EFileSequential (non-Rugged) file mode for write tests
// - may be overridden by +/-q command line switches
static TBool gFileSequentialModeOn = EFalse;

static TInt gFastCounterFreq;



static void RecursiveRmDir(const TDesC& aDes)
//
// Delete directory contents recursively
//
	{
	CDir* pD;
	TFileName n=aDes;
	n.Append(_L("*"));
	TInt r=TheFs.GetDir(n,KEntryAttMaskSupported,EDirsLast,pD);
	if (r==KErrNotFound || r==KErrPathNotFound)
		return;
	test_KErrNone(r);
	TInt count=pD->Count();
	TInt i=0;
	while (i<count)
		{
		const TEntry& e=(*pD)[i++];
		if (e.IsDir())
			{
			TFileName dirName;
			dirName.Format(_L("%S%S\\"),&aDes,&e.iName);
			RecursiveRmDir(dirName);
			}
		else
			{
			TFileName fileName;
			fileName.Format(_L("%S%S"),&aDes,&e.iName);
			r=TheFs.Delete(fileName);
			test_KErrNone(r);
			}
		}
	delete pD;
	r=TheFs.RmDir(aDes);
	test_KErrNone(r);
	}


void static ClearSessionDirectory()
//
// Delete the contents of F32-TST
//
	{
	TParse sessionPath;
	TInt r=TheFs.Parse(_L("\\F32-TST\\"),_L(""),sessionPath);
	test_KErrNone(r);
	RecursiveRmDir(sessionPath.FullName());
	r=TheFs.MkDir(sessionPath.FullName());
	test_KErrNone(r);
	}


static void DoTestFileRead(TInt aBlockSize, TInt aFileSize = KMaxFileSize, TBool aReRead = EFalse)
//
// Do Read Test
//
	{
	// Create test data
//	test.Printf(_L("Creating test file..."));
	TInt writeBlockLen = aFileSize > DataBuf.MaxSize() ? DataBuf.MaxSize() : aFileSize;
	DataBuf.SetLength(writeBlockLen);
#if defined(_DEBUG)
	for (TInt m = 0; m < DataBuf.Length(); m++)
		DataBuf[m] = TText8(m % 256);
#endif
	// To allow this test to run on a non-preq914 branch :
	enum {EFileWriteDirectIO = 0x00001000};
	TInt r = File.Create(TheFs, _L("READTEST"), EFileStream | EFileWriteDirectIO);
	test_KErrNone(r);
	TInt count = aFileSize / DataBuf.Length();
	while (count--)
		File.Write(DataBuf);
//	test.Printf(_L("done\n"));
	File.Close();

	enum {EFileReadBuffered = 0x00002000, EFileReadDirectIO = 0x00004000};
	r = File.Open(TheFs, _L("READTEST"), EFileStream | (gReadCachingOn ? EFileReadBuffered : EFileReadDirectIO));
	test_KErrNone(r);

//	const TInt maxReadCount = aFileSize / aBlockSize;
	TUint functionCalls = 0;

#if defined SYMBIAN_TEST_COPY
	// To allow this test to run on a non-preq914 branch :
	enum {EFileWriteDirectIO = 0x00001000};
	TInt r = File2.Replace(TheFs, _L("WRITETEST"), EFileStream | EFileWriteDirectIO);
	test_KErrNone(r);
#endif

	TTime startTime(0);
	TTime endTime(0);

	// we stop after the entire file has been read or after 10 seconds, whichever happens sooner
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;


	TUint initTicks = 0;
	TUint finalTicks = 0;

	// if aReRead file is set, then read file twice
	for (TInt n=0; n<(aReRead?2:1); n++)
		{
		functionCalls = 0;

		const TInt readLen = (aReRead && n == 0) ? writeBlockLen : aBlockSize;
		const TInt maxReadCount = aFileSize / readLen;

		TInt pos = 0;
		File.Seek(ESeekStart, pos);

		timer.After(reqStat, 10000000); // After 10 secs
		startTime.HomeTime();
		initTicks = User::FastCounter();

		for (TInt i = 0; i<maxReadCount && reqStat==KRequestPending; i++)
			{
//			test.Printf(_L("Read %d\n"),i);
//			for (TInt a = 0; a < 512; a++)
//				test.Printf(_L("%d"),DataBuf[a]);

			TInt r = File.Read(DataBuf, readLen);
			test_KErrNone(r);

			if (DataBuf.Length() == 0)
				break;

#if defined SYMBIAN_TEST_COPY
			r = File2.Write(DataBuf, readLen);
			test_KErrNone(r);
#endif
			functionCalls++;

#if defined(_DEBUG)
//			for (TInt a = 0; a < 512; a++)
//				test.Printf(_L("%d"),DataBuf[a]);

			for (TInt j = 0; j < DataBuf.Size(); j++)
				test(DataBuf[j] == (j + i * readLen) % 256);
#endif

			}

		finalTicks = User::FastCounter();
		endTime.HomeTime();
		timer.Cancel();
		}

	TInt dataTransferred = functionCalls * aBlockSize;

//	TTimeIntervalMicroSeconds duration = endTime.MicroSecondsFrom(startTime);
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;

	TReal transferRate =
		TReal32(dataTransferred) /
		TReal(duration.Int64()) * TReal(1000000) / TReal(K1K); // KB/s
	test.Printf(_L("Read %7d bytes in %7d byte blocks:\t%11.3f KBytes/s (%d microsecs)\n"),
		dataTransferred, aBlockSize, transferRate, endTime.MicroSecondsFrom(startTime).Int64());


	timer.Close();
#if defined SYMBIAN_TEST_COPY
	File2.Close();
#endif

	File.Close();
	r = TheFs.Delete(_L("READTEST"));
	test_KErrNone(r);
	return;
	}


static void TestFileRead(TInt aFileSize = KMaxFileSize, TBool aMisalignedReadWrites = EFalse, TBool aReRead = EFalse)
//
// Benchmark read method
//
	{
	ClearSessionDirectory();
	test.Next(_L("Benchmark read method"));

	_LIT(KLitReadOnce,"Read-once");
	_LIT(KLitReRead,"Re-read");
	test.Printf(_L("FileSize %d, MisalignedReadWrites %d %S\n"), aFileSize, aMisalignedReadWrites, aReRead ? &KLitReRead : &KLitReadOnce);
	TInt misalignedOffset = aMisalignedReadWrites ? 1 : 0;

#if defined (SYMBIAN_TEST_EXTENDED_BUFFER_SIZES)
	DoTestFileRead(1+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(2+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(4+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(8+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(16+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(32+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(64+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(128+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(256+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(512+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(2 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(4 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(8 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(16 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(32 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(64 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(128 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(256 * 1024+misalignedOffset, aFileSize, aReRead);
#ifndef __WINS__	// Block sizes are too large for the emulator
	DoTestFileRead(512 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(1024 * 1024+misalignedOffset, aFileSize, aReRead);
#endif
#else
	DoTestFileRead(16+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(512+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(4096+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(32768+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(64 * 1024+misalignedOffset, aFileSize, aReRead);
	DoTestFileRead(K1M+misalignedOffset, aFileSize, aReRead);
#endif

	}


static TInt FloatingPointLoop(TAny* funcCount)
	{
	TUint& count = *(TUint*) funcCount;
	TReal eq = KPi;

	FOREVER
		{
		eq *= eq;
		count++;
		}

	}


static void DoTestFileReadCPU(TInt aBlockSize)
//
// Benchmark CPU utilisation for Read method
//
// Read operations are performed for 10 seconds whilst a second thread executes floating point calculations
// The higher the number of calculations the less amount of CPU time has been used by the Read method.
//
	{
	enum {EFileReadBuffered = 0x00002000, EFileReadDirectIO = 0x00004000};
	TInt r = File.Open(TheFs, _L("READCPUTEST"), EFileStream | (gReadCachingOn ? EFileReadBuffered : EFileReadDirectIO));
	test_KErrNone(r);

	TInt pos = 0;

	TUint functionCalls = 0;
	TUint fltPntCalls = 0;
	RThread fltPntThrd;

	TBuf<6> buf = _L("Floaty");
	fltPntThrd.Create(buf, FloatingPointLoop, KDefaultStackSize, KHeapSize, KHeapSize, (TAny*) &fltPntCalls);

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;

	TUint initTicks = 0;
	TUint finalTicks = 0;

	timer.After(reqStat, KFloatingPointTestTime); // After 10 secs
	initTicks = User::FastCounter();

	// up the priority of this thread so that we only run the floating point thread when this thread is idle
	RThread				thisThread;
	thisThread.SetPriority(EPriorityMuchMore);

	TRequestStatus req;
	fltPntThrd.Logon(req);

	fltPntThrd.Resume();

	for (TInt i = 0; reqStat==KRequestPending; i++)
		{
		TInt r = File.Read(pos, DataBuf, aBlockSize);
		test_KErrNone(r);

		pos += aBlockSize;
		if (pos > KMaxFileSize-aBlockSize)
			pos = 0;

		functionCalls++;
		}

	TUint fltPntCallsFinal = fltPntCalls;
	fltPntThrd.Kill(KErrNone);

	finalTicks = User::FastCounter();

	fltPntThrd.Close();
	User::WaitForRequest(req);

	TInt dataTransferred = functionCalls * aBlockSize;

	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;

	TReal transferRate =  TReal32(dataTransferred) /
						 TReal(duration.Int64()) * TReal(1000000) / TReal(K1K); // KB/s

	test.Printf(_L("Read %7d bytes in %7d byte blocks:\t%11.3f KBytes/s; %d Flt Calcs\n"),
				    dataTransferred, aBlockSize, transferRate, fltPntCallsFinal);

	timer.Close();

	File.Close();

	return;
	}


static void TestFileReadCPU(TBool aMisalignedReadWrites = EFalse)
//
// Benchmark CPU utilisation for Read method
//
	{
	ClearSessionDirectory();
	test.Next(_L("Benchmark Read method CPU Utilisation"));

	test.Printf(_L("MisalignedReadWrites %d\n"), aMisalignedReadWrites);
	TInt misalignedOffset = aMisalignedReadWrites ? 1 : 0;

	// Create test data
	test.Printf(_L("Creating test file..."));
	DataBuf.SetLength(KMaxFileSize);

	TInt r = File.Create(TheFs, _L("READCPUTEST"), EFileStream | EFileWriteDirectIO);
	test_KErrNone(r);

	File.Write(DataBuf);

	test.Printf(_L("done\n"));
	File.Close();

	DoTestFileReadCPU(1+misalignedOffset);
	DoTestFileReadCPU(2+misalignedOffset);
	DoTestFileReadCPU(4+misalignedOffset);
	DoTestFileReadCPU(8+misalignedOffset);
	DoTestFileReadCPU(16+misalignedOffset);
	DoTestFileReadCPU(32+misalignedOffset);
	DoTestFileReadCPU(64+misalignedOffset);
	DoTestFileReadCPU(128+misalignedOffset);
	DoTestFileReadCPU(256+misalignedOffset);
	DoTestFileReadCPU(512+misalignedOffset);
	DoTestFileReadCPU(1024+misalignedOffset);
	DoTestFileReadCPU(2 * 1024+misalignedOffset);
	DoTestFileReadCPU(4 * 1024+misalignedOffset);
	DoTestFileReadCPU(8 * 1024+misalignedOffset);
	DoTestFileReadCPU(16 * 1024+misalignedOffset);
	DoTestFileReadCPU(32 * 1024+misalignedOffset);
	DoTestFileReadCPU(64 * 1024+misalignedOffset);
	DoTestFileReadCPU(128 * 1024+misalignedOffset);
	DoTestFileReadCPU(256 * 1024+misalignedOffset);
#ifndef __WINS__	// Block sizes are too large for the emulator
	DoTestFileReadCPU(512 * 1024+misalignedOffset);
	DoTestFileReadCPU(K1M+misalignedOffset);
#endif

	r = TheFs.Delete(_L("READCPUTEST"));
	test_KErrNone(r);
	}


static void DoTestFileWrite(TInt aBlockSize, TInt aFileSize = KMaxFileSize, TBool aUpdate = EFalse)
//
// Do Write benchmark
//
	{
	DataBuf.SetLength(aBlockSize);
	const TInt maxWriteCount = aFileSize / aBlockSize;

	TFileName testDir(_L("?:\\F32-TST\\"));
	testDir[0] = (TText) gDriveToTest;
	TInt r = TheFs.MkDir(testDir);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);

	TFileName fileName;
	r = File.Temp(TheFs, testDir, fileName, EFileWrite | (gFileSequentialModeOn ? EFileSequential : 0) 
											| (gWriteCachingOn ? EFileWriteBuffered : EFileWriteDirectIO));
	test_KErrNone(r);

	if (aUpdate)
		{
		TInt r = File.SetSize(aFileSize);
		test_KErrNone(r);
		}

	TUint functionCalls = 0;

	TTime startTime;
	TTime endTime;
	TUint initTicks = 0;
	TUint finalTicks = 0;


	// we stop after the entire file has been written or after 10 seconds, whichever happens sooner
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;

	TInt pos = 0;
	File.Seek(ESeekStart, pos);

	timer.After(reqStat, 10000000); // After 10 secs

	startTime.HomeTime();
	initTicks = User::FastCounter();

	for (TInt i = 0 ; i<maxWriteCount && reqStat==KRequestPending; i++)
		{
		File.Write(pos, DataBuf, aBlockSize);

		pos += aBlockSize;
		if (pos > KMaxFileSize-aBlockSize)
			pos = 0;

		functionCalls++;
		}

	if (gFlushAfterWrite)
		{
		r = File.Flush();
		test_KErrNone(r)
		}

	// write file once only
	finalTicks = User::FastCounter();
	endTime.HomeTime();
//	TTimeIntervalMicroSeconds duration = endTime.MicroSecondsFrom(startTime);
	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;

	TInt dataTransferred = functionCalls * aBlockSize;
	TReal transferRate =
		TReal32(dataTransferred) /
		TReal(duration.Int64()) * TReal(1000000) / TReal(K1K); // KB/s
	test.Printf(_L("Write %7d bytes in %7d byte blocks:\t%11.3f KBytes/s (%d microsecs)\n"),
		dataTransferred, aBlockSize, transferRate, endTime.MicroSecondsFrom(startTime).Int64());

	timer.Close();

	File.Close();
	r = TheFs.Delete(fileName);
	test_KErrNone(r)

	return;
	}


static void TestFileWrite(TInt aFileSize = KMaxFileSize, TBool aMisalignedReadWrites = EFalse, TBool aUpdate = EFalse)
//
// Benchmark write method
//
	{
	ClearSessionDirectory();
	test.Next(_L("Benchmark write method"));

	_LIT(KLitUpdate,"update");
	_LIT(KLitAppend,"append");
	test.Printf(_L("FileSize %d %S MisalignedReadWrites %d\n"), aFileSize, aUpdate? &KLitUpdate : &KLitAppend, aMisalignedReadWrites);

	TInt misalignedOffset = aMisalignedReadWrites ? 1 : 0;

#if defined (SYMBIAN_TEST_EXTENDED_BUFFER_SIZES)
	DoTestFileWrite(1+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(2+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(4+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(8+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(16+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(32+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(64+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(128+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(256+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(512+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(2 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(4 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(8 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(16 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(32 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(64 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(128 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(256 * 1024+misalignedOffset, aFileSize, aUpdate);
#ifndef __WINS__	// Block sizes are too large for the emulator
	DoTestFileWrite(512 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(1024 * 1024+misalignedOffset, aFileSize, aUpdate);
#endif
#else
	DoTestFileWrite(16+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(512+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(4096+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(32768+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(64 * 1024+misalignedOffset, aFileSize, aUpdate);
	DoTestFileWrite(K1M+misalignedOffset, aFileSize, aUpdate);
#endif
	}



static void DoTestFileWriteCPU(TInt aBlockSize)
//
// Benchmark CPU utilisation for Write method
//
// Write operations are performed for 10 seconds whilst a second thread executes floating point calculations
// The higher the number of calculations the less amount of CPU time has been used by the Write method.
//
	{
	DataBuf.SetLength(aBlockSize);

	TFileName testDir(_L("?:\\F32-TST\\"));
	testDir[0] = (TText) gDriveToTest;
	TInt r = TheFs.MkDir(testDir);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);

	TFileName fileName;
	r = File.Temp(TheFs, testDir, fileName, EFileWrite | (gFileSequentialModeOn ? EFileSequential : 0)
											| (gWriteCachingOn ? EFileWriteBuffered : EFileWriteDirectIO));
	test_KErrNone(r);

	TUint functionCalls = 0;
	TUint fltPntCalls = 0;
	RThread fltPntThrd;

	TBuf<6> buf = _L("Floaty");
	fltPntThrd.Create(buf, FloatingPointLoop, KDefaultStackSize, KHeapSize, KHeapSize, (TAny*) &fltPntCalls);

	TUint initTicks = 0;
	TUint finalTicks = 0;

	// up the priority of this thread so that we only run the floating point thread when this thread is idle
	RThread				thisThread;
	thisThread.SetPriority(EPriorityMuchMore);

	TRequestStatus req;
	fltPntThrd.Logon(req);

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;

	TInt pos = 0;
	File.Seek(ESeekStart, pos);

	timer.After(reqStat, KFloatingPointTestTime);
	initTicks = User::FastCounter();

	fltPntThrd.Resume();

	for (TInt i = 0 ; reqStat==KRequestPending; i++)
		{
		File.Write(DataBuf, aBlockSize);
		functionCalls++;
		}
	TUint fltPntCallsFinal = fltPntCalls;

	fltPntThrd.Kill(KErrNone);

	finalTicks = User::FastCounter();

	fltPntThrd.Close();
	User::WaitForRequest(req);

	TTimeIntervalMicroSeconds duration = TInt64(finalTicks - initTicks) * TInt64(1000000) / TInt64(gFastCounterFreq) ;

	TInt dataTransferred = functionCalls * aBlockSize;
	TReal transferRate =  TReal32(dataTransferred) /
						 TReal(duration.Int64()) * TReal(1000000) / TReal(K1K); // KB/s

	test.Printf(_L("Write %7d bytes in %7d byte blocks:\t%11.3f KBytes/s; %d Flt Calcs\n"),
				    dataTransferred, aBlockSize, transferRate, fltPntCallsFinal);

	timer.Close();

	File.Close();
	r = TheFs.Delete(fileName);
	test_KErrNone(r)

	return;
	}


static void TestFileWriteCPU(TBool aMisalignedReadWrites = EFalse)
//
// Benchmark CPU utilisation for Write method
//
	{
	ClearSessionDirectory();
	test.Next(_L("Benchmark Write method CPU Utilisation"));

	test.Printf(_L("MisalignedReadWrites %d\n"), aMisalignedReadWrites);
	TInt misalignedOffset = aMisalignedReadWrites ? 1 : 0;

	DoTestFileWriteCPU(1+misalignedOffset);
	DoTestFileWriteCPU(2+misalignedOffset);
	DoTestFileWriteCPU(4+misalignedOffset);
	DoTestFileWriteCPU(8+misalignedOffset);
	DoTestFileWriteCPU(16+misalignedOffset);
	DoTestFileWriteCPU(32+misalignedOffset);
	DoTestFileWriteCPU(64+misalignedOffset);
	DoTestFileWriteCPU(128+misalignedOffset);
	DoTestFileWriteCPU(256+misalignedOffset);
	DoTestFileWriteCPU(512+misalignedOffset);
	DoTestFileWriteCPU(1024+misalignedOffset);
	DoTestFileWriteCPU(2 * 1024+misalignedOffset);
	DoTestFileWriteCPU(4 * 1024+misalignedOffset);
	DoTestFileWriteCPU(8 * 1024+misalignedOffset);
	DoTestFileWriteCPU(16 * 1024+misalignedOffset);
	DoTestFileWriteCPU(32 * 1024+misalignedOffset);
	DoTestFileWriteCPU(64 * 1024+misalignedOffset);
	DoTestFileWriteCPU(128 * 1024+misalignedOffset);
	DoTestFileWriteCPU(256 * 1024+misalignedOffset);
#ifndef __WINS__	// Block sizes are too large for the emulator
	DoTestFileWriteCPU(512 * 1024+misalignedOffset);
	DoTestFileWriteCPU(K1M+misalignedOffset);
#endif
	}


static void TestFileSeek()
//
// Benchmark file seek method
//
	{
	ClearSessionDirectory();
	test.Next(_L("RFile::Seek method"));
	TInt increment=1789; // random number > cluster size
	TInt count=0;
	TInt pos=0;

	// Create test file
	TBuf8<1024> testdata(1024);
	RFile f;
	TInt r=f.Create(TheFs,_L("SEEKTEST"),EFileStream);
	test_KErrNone(r);
	count=64;
	while (count--)
		f.Write(testdata);
	TInt fileSize=count*testdata.MaxLength();

	pos=0;
	count=0;
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,10000000); // After 10 secs
	while(reqStat==KRequestPending)
		{
		TInt dum=(pos+=increment)%fileSize;
		f.Seek(ESeekStart,dum);
		count++;
		}
	test.Printf(_L("RFile::Seek operations in 10 secs == %d\n"),count);
	timer.Close();

	f.Close();
	TheFs.Delete(_L("SEEKTEST"));
	}

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

static void CreateManyLargFiles(TInt aNumber)
//
// Make a directory with aNumber entries
//
	{
	RFile64 f;
	TInt maxEntry=aNumber;

	test.Printf(_L("Create a directory with %d entries\n"),aNumber);

	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	r=TheFs.MkDir(_L("\\F32-TST\\"));
	test((r==KErrNone)||(r==KErrAlreadyExists));
	r=TheFs.MkDir(_L("\\F32-TST\\BENCH_DELETE\\"));
	test((r==KErrNone)||(r==KErrAlreadyExists));
	TBuf8<8> WriteData =_L8("Wibbleuy");
	for (TInt i=0;i<maxEntry;i++)
		{
		TFileName baseName=_L("\\F32-TST\\BENCH_DELETE\\FILE");
		baseName.AppendNum(i);
		r=f.Replace(TheFs,baseName,EFileWrite);
		test_KErrNone(r);
		r = f.SetSize(K3GB);
		test_KErrNone(r);
		r=f.Write((K3GB-30),WriteData);
		test_KErrNone(r);
		f.Flush();
		f.Close();
		}

	test.Printf(_L("Test all entries have been created successfully\n"));
	TBuf8<8> ReadData;
	TInt64 Size=0;
	for (TInt j=0;j<=maxEntry;j++)
		{
		TFileName baseName=_L("\\F32-TST\\BENCH_DELETE\\FILE");
		baseName.AppendNum(j);

		TInt r=f.Open(TheFs,baseName,EFileRead);
		if (r!=KErrNone)
			{
			test_Value(r, r == KErrNotFound && j==maxEntry);
			return;
			}
		ReadData.FillZ();
		r=f.Read((K3GB-30),ReadData);
		test_KErrNone(r);
		test(f.Size(Size)==KErrNone);
		test(K3GB == Size);
		test(ReadData==WriteData);
		f.Close();
		}
	}


static void TestLargeFileDelete()
//
// This test require MMC/SD card size >=4GB-2 in size
//
	{
	ClearSessionDirectory();
	test.Next(_L("Benchmark delete large file"));

	TInt64 total=0;

	TInt cycles=1;

	TInt i=0;

	//check Disk space and decide how many files to create
	TVolumeInfo volInfo;
    TInt r;

    r = TheFs.Volume(volInfo);
    test_KErrNone(r);
    
	TInt numberOfFiles = (TUint)(volInfo.iFree/(K4GB -2));
#ifdef __WINS__
	// Fix a maximum number of large files to create on the emulator
	if (numberOfFiles > 5)
		numberOfFiles = 5;
#endif
	test.Printf(_L("Number of large files =%d \n"),numberOfFiles);

	if(numberOfFiles<=0)
		{
		test.Printf(_L("Large File delete is skipped \n"));
		return;
		}

	for (; i<cycles; i++)
		{
	//	Create many files
		CreateManyLargFiles(numberOfFiles);

		test.Next(_L("Time the delete"));
	//	Now delete them and time it
		TTime startTime;
		startTime.HomeTime();

		for (TInt index=0;index<numberOfFiles;index++)
			{
			TFileName baseName=_L("\\F32-TST\\BENCH_DELETE\\FILE");
			baseName.AppendNum(index);

			TInt r=TheFs.Delete(baseName);
			test_KErrNone(r);
			}

		TTime endTime;
		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken;
		timeTaken=endTime.MicroSecondsFrom(startTime);
		TInt64 time=timeTaken.Int64();
		total+=time;
		}
//	We deleted cycles*numberOfFiles files in total microseconds
	TInt64 fileDeleteTime=total/(numberOfFiles*cycles);
	test.Next(_L("Benchmarked RFs::Delete()"));
	test.Printf(_L("Delete time per file = %d microseconds\n"),fileDeleteTime);

	CFileMan* fMan=CFileMan::NewL(TheFs);
	total=0;

	cycles=1;
	i=0;
	for (; i<cycles; i++)
		{
	//	Create many files
		CreateManyLargFiles(numberOfFiles);

		test.Next(_L("Time the delete"));
	//	Now delete them and time it
		TTime startTime;
		startTime.HomeTime();

		for (TInt index=0;index<numberOfFiles;index++)
			{
			TInt r=fMan->Delete(_L("\\F32-TST\\BENCH_DELETE\\FILE*"));
			test_Value(r, r == KErrNone || r==KErrNotFound);
			}

		TTime endTime;
		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken;
		timeTaken=endTime.MicroSecondsFrom(startTime);
		TInt64 time=timeTaken.Int64();
		total+=time;
		}
//	We deleted cycles*numberOfFiles files in total microseconds
	fileDeleteTime=total/(numberOfFiles*cycles);
	test.Next(_L("Benchmarked CFileMan::Delete()"));
	test.Printf(_L("Delete time per file = %d microseconds\n"),fileDeleteTime);

	delete fMan;
}

#endif //SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API


static void CreateManyFiles(TInt aNumber)
//
// Make a directory with aNumber entries
//
	{
	RFile f;
	TInt maxEntry=aNumber;

	test.Printf(_L("Create a directory with %d entries\n"),aNumber);

	TFileName sessionPath;
	TInt r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	r=TheFs.MkDir(_L("\\F32-TST\\"));
	test((r==KErrNone)||(r==KErrAlreadyExists));
	r=TheFs.MkDir(_L("\\F32-TST\\BENCH_DELETE\\"));
	test((r==KErrNone)||(r==KErrAlreadyExists));

	for (TInt i=0;i<maxEntry;i++)
		{
		TFileName baseName=_L("\\F32-TST\\BENCH_DELETE\\FILE");
		baseName.AppendNum(i);
		r=f.Replace(TheFs,baseName,EFileRead);
		test_KErrNone(r);
		r=f.Write(_L8("Wibble"));
		test_KErrNone(r);
		f.Close();
		}

	test.Printf(_L("Test all entries have been created successfully\n"));
	for (TInt j=0;j<=maxEntry;j++)
		{
		TFileName baseName=_L("\\F32-TST\\BENCH_DELETE\\FILE");
		baseName.AppendNum(j);
		TInt r=f.Open(TheFs,baseName,EFileRead);
		if (r!=KErrNone)
			{
			test_Value(r, r == KErrNotFound && j==maxEntry);
			return;
			}
		TBuf8<16> data;
		r=f.Read(data);
		test_KErrNone(r);
		test(data==_L8("Wibble"));
		f.Close();
		}
	}


static void TestFileDelete()
//
//
//
	{
	ClearSessionDirectory();
	test.Next(_L("Benchmark delete"));

	TInt64 total=0;
	TInt numberOfFiles=100;
	TInt cycles=1;

	TInt i=0;
	for (; i<cycles; i++)
		{
	//	Create many files
		CreateManyFiles(numberOfFiles);

		test.Next(_L("Time the delete"));
	//	Now delete them and time it
		TTime startTime;
		startTime.HomeTime();

		for (TInt index=0;index<numberOfFiles;index++)
			{
			TFileName baseName=_L("\\F32-TST\\BENCH_DELETE\\FILE");
			baseName.AppendNum(index);

			TInt r=TheFs.Delete(baseName);
			test_KErrNone(r);
			}

		TTime endTime;
		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken;
		timeTaken=endTime.MicroSecondsFrom(startTime);
		TInt64 time=timeTaken.Int64();
		total+=time;
		}
//	We deleted cycles*numberOfFiles files in total microseconds
	TInt64 fileDeleteTime=total/(numberOfFiles*cycles);
	test.Next(_L("Benchmarked RFs::Delete()"));
	test.Printf(_L("Delete time per file = %d microseconds\n"),fileDeleteTime);

	CFileMan* fMan=CFileMan::NewL(TheFs);
	total=0;
	numberOfFiles=100;
	cycles=1;
	i=0;
	for (; i<cycles; i++)
		{
	//	Create many files
		CreateManyFiles(numberOfFiles);

		test.Next(_L("Time the delete"));
	//	Now delete them and time it
		TTime startTime;
		startTime.HomeTime();

		for (TInt index=0;index<numberOfFiles;index++)
			{
			TInt r=fMan->Delete(_L("\\F32-TST\\BENCH_DELETE\\FILE*"));
			test_Value(r, r == KErrNone || r==KErrNotFound);
			}

		TTime endTime;
		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken;
		timeTaken=endTime.MicroSecondsFrom(startTime);
		TInt64 time=timeTaken.Int64();
		total+=time;
		}
//	We deleted cycles*numberOfFiles files in total microseconds
	fileDeleteTime=total/(numberOfFiles*cycles);
	test.Next(_L("Benchmarked CFileMan::Delete()"));
	test.Printf(_L("Delete time per file = %d microseconds\n"),fileDeleteTime);

	delete fMan;
}


/*
TInt maxDirEntry=200;
static void TestDirRead()
//
// Benchmark directory read method
//
	{

	ClearSessionDirectory();
	test.Next(_L("Benchmark directory read method"));
// Create one test entry
	RFile f;
	f.Create(TheFs,_L("ONE.XXX"),EFileStream);
	f.Close();
	TTime start;
	start.HomeTime();
	TInt i=0;
	for (i=0;i<maxDirEntry;i++)
		{
		CDir* dirPtr;
		TheFs.GetDir(_L("*"),KEntryAttMaskSupported,ESortByName,dirPtr);
		delete dirPtr;
		}
	TTime end;
	end.HomeTime();
	DirReadOne=end.MicroSecondsFrom(start);
// Create lost of test entries
	for (i=0;i<maxDirEntry;i++)
		{
		TBuf<12> baseName(_L("MANY"));
		baseName.AppendNum(i,EHex);
		baseName.Append(_L(".TXT"));
		RFile f;
		f.Create(TheFs,baseName,EFileStream);
		f.Close();
		}
	start.HomeTime();
	CDir* dirPtr;
	TheFs.GetDir(_L("*"),KEntryAttMaskSupported,ESortByName,dirPtr);
	delete dirPtr;
	end.HomeTime();
	DirReadMany=end.MicroSecondsFrom(start);
// Select one entry from lots
	start.HomeTime();
	TheFs.GetDir(_L("*.XXX"),KEntryAttMaskSupported,ESortByName,dirPtr);
	end.HomeTime();
	test(dirPtr->Count()==1);
	delete dirPtr;
	DirMatchOne=end.MicroSecondsFrom(start);
	}


void static PrintDirResults()
//
// Print results of Directory Benchmark
//
	{
	test.Printf(_L("\nBenchmark: Dir Results\n"));
	test.Printf(_L("Read one entry %d times = %d ms\n"),maxDirEntry,DirReadOne.Int64().GetTInt()/1000);
	test.Printf(_L("Read %d entries = %d ms\n"),maxDirEntry,DirReadMany.Int64().GetTInt()/1000);
	test.Printf(_L("Match 1 entry from %d entries = %d ms\n"),maxDirEntry,DirMatchOne.Int64().GetTInt()/1000);
	test.Printf(_L("Press Enter to continue\n\n"));
	test.Getch();
	}
*/


static void TestMkDir()
	{
	test.Next(_L("Benchmark MkDir"));
	ClearSessionDirectory();

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	startTime.HomeTime();

	const TInt KNumDirEntries = 100;
	for (TInt n=0; n<KNumDirEntries; n++)
		{
		TFileName dirName = _L("\\F32-TST\\DIR_");
		dirName.AppendNum(n);
		dirName.Append(_L("\\"));
		TInt r = TheFs.MkDir(dirName);
		test_KErrNone(r);
		}

	endTime.HomeTime();
	timeTaken=endTime.MicroSecondsFrom(startTime);
	TInt timeTakenInMs = I64LOW(timeTaken.Int64() / 1000);
	test.Printf(_L("Time taken to create %d entries = %d ms\n"), KNumDirEntries, timeTakenInMs);
	}


// Allocate Data Buffers for Read/Write Tests
void AllocateBuffers()
	{
	test.Printf(_L("Allocate Buffers -"));

	if (gFragSharedMemory || gSharedMemory)
		{
		test.Printf(_L("Shared Memory\n"));

		RLoader l;
		test(l.Connect()==KErrNone);
		test(l.CancelLazyDllUnload()==KErrNone);
		l.Close();

		test.Printf(_L("Initialise\n"));
		TInt r = UserHal::PageSizeInBytes(PageSize);
		test_KErrNone(r);

		test.Printf(_L("Loading test driver\n"));
		r = User::LoadLogicalDevice(KSharedChunkLddName);
		test_Value(r, r == KErrNone || r==KErrAlreadyExists);

		test.Printf(_L("Opening channel\n"));
		r = Ldd.Open();
		test_KErrNone(r);

		test.Printf(_L("Create chunk\n"));

		TUint aCreateFlags = EMultiple|EOwnsMemory;
	    TCommitType aCommitType = EContiguous;

	    TUint TotalChunkSize = ChunkSize;  // rounded to nearest Page Size

		TUint ChunkAttribs = TotalChunkSize|aCreateFlags;
		r = Ldd.CreateChunk(ChunkAttribs);
		test_KErrNone(r);

		if (gSharedMemory)
			{
		    test.Printf(_L("Commit Contigouos Memory\n"));
		    r = Ldd.CommitMemory(aCommitType,TotalChunkSize);
			test_KErrNone(r);
			}
		else
			{
			test.Printf(_L("Commit Fragmented Memory\n"));

			// Allocate Pages in reverse order to maximise memory fragmentation
			TUint i = ChunkSize;
			do
				{
				i-=PageSize;
				test.Printf(_L("Commit %d\n"), i);
				r = Ldd.CommitMemory(aCommitType|i,PageSize);
				test_KErrNone(r);
				}while (i>0);
/*
			for (TInt i = (ChunkSize-PageSize); i>=0; )
				{
				test.Printf(_L("Commit %d\n"), i);
				r = Ldd.CommitMemory(aCommitType|i,PageSize);
				test_KErrNone(r);
				i-=PageSize;
				}
*/
			}

		test.Printf(_L("\nOpen user handle\n"));
		r = Ldd.GetChunkHandle(TheChunk);
		test_KErrNone(r);

		DataBuf.Set(TheChunk.Base(),KMaxFileSize, KMaxFileSize);
		}
	else
		{
		test.Printf(_L("Heap Memory\n"));
		DataBufH = HBufC8::New(KMaxFileSize);
		test(DataBufH != NULL);

		DataBuf.Set(DataBufH->Des());
		}
	}


void DeAllocateBuffers()
	{
	test.Printf(_L("DeAllocate Buffers -"));

	if (gFragSharedMemory || gSharedMemory)
	{
		test.Printf(_L("Shared Memory\n"));
		test.Printf(_L("Close user chunk handle\n"));
		TheChunk.Close();

		test.Printf(_L("Close kernel chunk handle\n"));
		TInt r = Ldd.CloseChunk();
		test_Value(r, r == 1);

		test.Printf(_L("Check chunk is destroyed\n"));
		r = Ldd.IsDestroyed();
		test_Value(r, r == 1);

		test.Printf(_L("Close test driver\n"));
		Ldd.Close();
		}
	else
		{
		test.Printf(_L("Heap Memory\n"));
		test.Printf(_L("Delete Heap Buffer\n"));
		delete DataBufH;
		}
	}

void ParseCommandLine()
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);

    for (TPtrC token=lex.NextToken(); token.Length() != 0;token.Set(lex.NextToken()))
		{
		if (token.MatchF(RProcess().FileName())==0)
			{
			continue;
			}

		if (token.CompareF(_L("-m"))== 0)
			{
			gMisalignedReadWrites = ETrue;
			continue;
			}
		if (token.CompareF(_L("-r"))== 0)
			{
			gReadCachingOn = EFalse;
			continue;
			}
		if (token.CompareF(_L("+r"))== 0)
			{
			gReadCachingOn = ETrue;
			continue;
			}
		if (token.CompareF(_L("-w"))== 0)
			{
			gWriteCachingOn = EFalse;
			continue;
			}
		if (token.CompareF(_L("+w"))== 0)
			{
			gWriteCachingOn = ETrue;
			continue;
			}

		if (token.CompareF(_L("-f"))== 0)
			{
			gFlushAfterWrite = EFalse;
			continue;
			}
		if (token.CompareF(_L("+f"))== 0)
			{
			gFlushAfterWrite = ETrue;
			continue;
			}
		if (token.CompareF(_L("+s"))== 0)
			{
			gSharedMemory = ETrue;
			continue;
			}
		if (token.CompareF(_L("+x"))== 0)
			{
			gFragSharedMemory = ETrue;
			continue;
			}
		
		if (token.CompareF(_L("+q"))== 0)
			{
			gFileSequentialModeOn = ETrue;
			continue;
			}
		if (token.CompareF(_L("-q"))== 0)
			{
			gFileSequentialModeOn = EFalse;
			continue;
			}

		test.Printf(_L("CLP=%S\n"),&token);

		if(token.Length()!=0)
			{
			gDriveToTest=token[0];
			gDriveToTest.UpperCase();
			}
		else
			gDriveToTest='C';

#if defined SYMBIAN_TEST_COPY
		token.Set(lex.NextToken());
		if(token.Length()!=0)
			{
			gDriveToTest2=token[0];
			gDriveToTest2.UpperCase();
			}
		else
			gDriveToTest2='C';
		test.Printf(_L("CLP2=%S\n"),&token);
#endif

		}
	}


GLDEF_C void CallTestsL(void)
//
// Call all tests
//
	{
	test.Title();
	test.Start(_L("Start Benchmarking ..."));

	test.Next(gSessionPath);

	ParseCommandLine();

	AllocateBuffers();
	RProcess().SetPriority(EPriorityBackground);

	TInt r = HAL::Get(HAL::EFastCounterFrequency, gFastCounterFreq);
	test_KErrNone(r);
	test.Printf(_L("HAL::EFastCounterFrequency %d\n"), gFastCounterFreq);

	test.Printf(_L("gReadCachingOn %d  gWriteCachingOn %d gFlushAfterWrite %d gFileSequentialModeOn %d\n"),
				gReadCachingOn, gWriteCachingOn, gFlushAfterWrite, gFileSequentialModeOn);

	TestFileSeek();

	// read once
	TestFileRead(KMaxFileSize, gMisalignedReadWrites, EFalse);

	// re-read
	TestFileRead(KMaxFileSize, gMisalignedReadWrites, ETrue);

	TestFileReadCPU(gMisalignedReadWrites);

	// append to file
	TestFileWrite(KMaxFileSize, gMisalignedReadWrites, EFalse);

	// update (overwrite) file
	TestFileWrite(KMaxFileSize, gMisalignedReadWrites, ETrue);

	TestFileWriteCPU(gMisalignedReadWrites);

	TestFileDelete();

//	TestDirRead();
//	PrintDirResults();
#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TestLargeFileDelete();
#endif //SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

	TestMkDir();

	RecursiveRmDir(gSessionPath);

	DeAllocateBuffers();

	test.End();
	test.Close();
	}
