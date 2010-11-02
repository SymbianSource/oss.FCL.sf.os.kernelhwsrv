// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_filecache.cpp
// 
//
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h> 
#include <f32dbg.h>
#include "t_server.h"
#include <e32twin.h>
#include <e32rom.h>
#include <u32hal.h>


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FILECACHE-0189
//! @SYMTestType        UT
//! @SYMPREQ            PREQ914
//! @SYMTestCaseDesc    Unit tests for fair scheduling, read caching and lazy writing
//! @SYMTestActions     0   setup the environment to execute the tests 
//!						1 	Reads a file with different blocksizes
//!						2	Write a file with different blocksizes
//!						3	Small reads while controlling the cache 
//!						4	Read operations with and without read ahead
//!						5 	Write operations with write buffer
//! @SYMTestExpectedResults finishes if the implementation of PREQ914 behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Critical
//----------------------------------------------------------------------------------------------


//#define SYMBIAN_TEST_EXTENDED_BUFFER_SIZES


GLDEF_D RTest test(_L("T_FILECACHE"));

GLDEF_D	RFs TheFs;
GLDEF_D TChar gDriveToTest;
GLDEF_D TInt gDrive;
GLDEF_D TFileName gSessionPath;

GLDEF_D TVolumeInfo gVolInfo;	// volume info for current drive
GLDEF_D TFileCacheFlags gDriveCacheFlags = TFileCacheFlags(0);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
static TFileCacheConfig gFileCacheConfig;
static TBool gDisplayCacheFlags = EFalse;
static TBool gWriteCacheFlags = EFalse;
static TBool gRomPaged = EFalse;
#endif
static TBool gRunTests = ETrue;
static TBool gRunUnitTests = ETrue;
static TBool gRunPerformanceTests = EFalse;
static TBool gRunManualTests = EFalse;


LOCAL_D TInt KMaxFileSize = 768 * 1024;

// Chosing a file size of 128K ensures entire file will be cached
//LOCAL_D TInt KMaxFileSize = 128 * 1024;

LOCAL_D TBuf8<256*1024> DataBuf;

const TInt KBufSize = 513 * 1024 - 16;
HBufC8* gBuf = NULL;


LOCAL_D TPtr8 gBufPtr(NULL, 0);

const TReal KOneK = 1024;
const TReal KOneMeg = 1024 * KOneK;

const TInt KSegmentSize = 4096;
const TInt KSegmentSizeMask = (4096-1);

GLDEF_D template <class C>
GLDEF_C TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void PrintFileCacheStats(TFileCacheStats& fileCacheStats, TBool aDisplay = ETrue)
	{
	TInt r = controlIo(TheFs,gDrive, KControlIoFileCacheStats, fileCacheStats);
	test_KErrNone(r);
	if (!aDisplay)
		return;
	test.Printf(_L("File cache: Cachelines (free %d, used %d), Segments(allocated %d locked %d). Closed files(%d) Holes %d Failures (commit %d Lock %d)\n"), 
		fileCacheStats.iFreeCount, fileCacheStats.iUsedCount, fileCacheStats.iAllocatedSegmentCount,fileCacheStats.iLockedSegmentCount,fileCacheStats.iFilesOnClosedQueue, fileCacheStats.iHoleCount, fileCacheStats.iCommitFailureCount, fileCacheStats.iLockFailureCount);
	test.Printf(_L("File cache: iUncachedPacketsRead %d iUncachedBytesRead %d iUncachedPacketsWritten %d iUncachedBytesWritten %d\n"), 
		fileCacheStats.iUncachedPacketsRead,
		fileCacheStats.iUncachedBytesRead,
		fileCacheStats.iUncachedPacketsWritten,
		fileCacheStats.iUncachedBytesWritten);
	}

void PrintFileCacheConfig(TFileCacheConfig& aFileCacheConfig, TBool aDisplay = ETrue)
	{
	TInt r = controlIo(TheFs,gDrive, KControlIoFileCacheConfig, aFileCacheConfig);
	test_KErrNone(r);
	if (!aDisplay)
		return;
	
	test.Printf(_L("File cache:\nDrive %c\nFlags %08X\nFileCacheReadAsync %d\nFairSchedulingLen %d\nCacheSize %d\nMaxReadAheadLen %d\nClosedFileKeepAliveTime %d\nDirtyDataFlushTime %d"), 
		aFileCacheConfig.iDrive + 'A',
		aFileCacheConfig.iFlags,
		aFileCacheConfig.iFileCacheReadAsync,
		aFileCacheConfig.iFairSchedulingLen,
		aFileCacheConfig.iCacheSize,
		aFileCacheConfig.iMaxReadAheadLen,
		aFileCacheConfig.iClosedFileKeepAliveTime,
		aFileCacheConfig.iDirtyDataFlushTime);
	
	}

void TestDirtyDataWrittenToDisk(TFileCacheStats& fileCacheStats)
	{
	test.Next(_L("test dirty data has been written to disk"));
	// wait a maximum of double KDefaultDirtyDataFlushTime for dirty data to be flushed
	const TInt KWaitTime = 250;	// 250 milisecs
	for (TInt n=0; n<(gFileCacheConfig.iDirtyDataFlushTime/1000)<<1 ; n+= KWaitTime)
		{
		test.Printf(_L("After %d milisecs : "), n );
		PrintFileCacheStats(fileCacheStats);
		User::After(KWaitTime * 1000);		// wait 100 ms
		if (fileCacheStats.iLockedSegmentCount == 0)
			break;
		}
	PrintFileCacheStats(fileCacheStats);
	test(fileCacheStats.iLockedSegmentCount == 0);
	}
#endif 

void TestsInit()
	{
	gBuf = HBufC8::NewL(KBufSize);
	test(gBuf!=NULL);
	gBufPtr.Set(gBuf->Des());
	}
void TestsEnd()
	{
	delete gBuf;
	gBuf = NULL;
	}

LOCAL_C void SetSessionPath(TInt aDrive)
	{
	gSessionPath=_L("?:\\F32-TST\\");
	TChar driveLetter;
	TInt r=TheFs.DriveToChar(aDrive,driveLetter);
	test_KErrNone(r);
	gSessionPath[0]=(TText)driveLetter;
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	}

LOCAL_C void PrintFileMode(TUint aFileMode)
	{
	TBuf<80> buf;

	buf.Format(_L("FileMode = %08X"), aFileMode);

	if (aFileMode & EFileWriteBuffered)
		buf.Append(_L(", EFileWriteBuffered"));

	if (aFileMode & EFileWriteDirectIO)
		buf.Append(_L(", EFileWriteDirectIO"));

	if (aFileMode & EFileReadBuffered)
		buf.Append(_L(", EFileReadBuffered"));

	if (aFileMode & EFileReadDirectIO)
		buf.Append(_L(", EFileReadDirectIO"));

	if (aFileMode & EFileReadAheadOn)
		buf.Append(_L(", EFileReadAheadOn"));

	if (aFileMode & EFileReadAheadOff)
		buf.Append(_L(", EFileReadAheadOff"));

	buf.Append(_L("\n"));
	test.Printf(buf);
	}

void FillBuffer(TDes8& aBuffer, TInt aLength)
	{
	test (aBuffer.MaxLength() >= aLength);
	for(TInt i=0; i<aLength; i+=2)
		{
		aBuffer[i]=(TUint8) (i >> 8);
		aBuffer[i+1]=(TUint8) i;
		}
	}

void TestBufferFail(TDes8& aBuffer, TInt aPos, TInt aLength)
	{
	test.Printf(_L("TestBuffer failed at pos %d len %d\n"), aPos, aLength);

	#define PRINTCH(ch) ((ch >= 0x20 && ch < 0x7F)?ch:' ')
	TInt startPos = Max(0, aPos - 64);
	TInt endPos = startPos + 64;

	for(TInt n=startPos; n<=endPos; n+=16)
		RDebug::Print(_L("%08X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X  [%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c]"), 
			//&aBuffer[aPos+n],
			n,
			aBuffer[n+0], aBuffer[n+1], aBuffer[n+2], aBuffer[n+3], aBuffer[n+4], aBuffer[n+5], aBuffer[n+6], aBuffer[n+7], aBuffer[n+8], aBuffer[n+9], aBuffer[n+10], aBuffer[n+11], aBuffer[n+12], aBuffer[n+13], aBuffer[n+14], aBuffer[n+15],
			PRINTCH(aBuffer[n+0]), PRINTCH(aBuffer[n+1]), PRINTCH(aBuffer[n+2]), PRINTCH(aBuffer[n+3]), PRINTCH(aBuffer[n+4]), PRINTCH(aBuffer[n+5]), PRINTCH(aBuffer[n+6]), PRINTCH(aBuffer[n+7]), PRINTCH(aBuffer[n+8]), PRINTCH(aBuffer[n+9]), PRINTCH(aBuffer[n+10]), PRINTCH(aBuffer[n+11]), PRINTCH(aBuffer[n+12]), PRINTCH(aBuffer[n+13]), PRINTCH(aBuffer[n+14]), PRINTCH(aBuffer[n+15]));

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		TInt x;
		TInt r = controlIo(TheFs,gDrive, KControlIoFileCacheDump, x);
		test_KErrNone(r);
#endif
	test (0);
	}

void TestBuffer(TDes8& aBuffer, TInt aPos, TInt aLength)
	{
	TInt pos = aPos;
	for(TInt i=0; i<aLength; i++, pos++)
		{
		if (pos & 1)
			{
			if (aBuffer[pos] != (TUint8) pos-1)
				TestBufferFail(aBuffer, pos, aLength);
			}
		else
			{
			if (aBuffer[pos] != (TUint8) (pos >> 8))
				TestBufferFail(aBuffer, pos, aLength);
			}
		}
	}

TInt FreeRam()
	{
	// wait for any async cleanup in the supervisor to finish first...
	UserSvr::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);

	TMemoryInfoV1Buf meminfo;
	UserHal::MemoryInfo(meminfo);
	return meminfo().iFreeRamInBytes;
	}

void LowMemoryTest()
	{
	TInt fileSize = 0;
	
	const TInt KWriteLen = 128*1024;
	test.Next(_L("Test appending to a file with low memory"));
	gBufPtr.SetLength(KBufSize);

	RFile f;
	TFileName testFile   = _L("TEST.BIN");

	TInt r = f.Replace(TheFs, testFile, EFileWrite | EFileWriteBuffered);
	test_KErrNone(r);

	TInt pos = 0;

	TPtrC8 writePtr;
	writePtr.Set(gBufPtr.MidTPtr(pos, KWriteLen));

	r = f.Write(pos, writePtr);
	test_KErrNone(r);
	pos+= writePtr.Length();

	r = f.Size(fileSize);
	test_KErrNone(r);
	test_Equal(fileSize,pos);



	TUint freeRam = FreeRam();
	const TInt KPageSize=4096;
	freeRam = (freeRam + KPageSize -1) & ~(KPageSize-1);
	test.Printf(_L("FreeRam = %d"), freeRam);

	RChunk chunk;
	TChunkCreateInfo chunkInfo;
	chunkInfo.SetDisconnected(0, 0, freeRam);
	chunkInfo.SetPaging(TChunkCreateInfo::EUnpaged);
	test_KErrNone(chunk.Create(chunkInfo));

	test.Printf(_L("Gobbling all of memory..."));
	
	TUint commitEnd;
	for (commitEnd = 0; commitEnd < freeRam; commitEnd += KPageSize) 
		{
		r = chunk.Commit(commitEnd,KPageSize);
		if (r != KErrNone)
			break;
		
		}
	test.Printf(_L("commitEnd %d, r %d"), commitEnd, r);
	test_Value(r, r == KErrNoMemory || r == KErrNone);

	test.Printf(_L("FreeRam = %d"), FreeRam());

	pos-= KSegmentSize;
	writePtr.Set(gBufPtr.MidTPtr(pos, KWriteLen));

	test.Printf(_L("Writing to file..."));

	// now we have gobbled all or most of memory, the next write can fail
	// if it does keep decommitting memory until it succeeds and then test that the file size is correct
	commitEnd = 0;
	do {

		r = f.Write(pos, writePtr);
		test_Value(r, r == KErrNoMemory || r == KErrNone);
		if (r == KErrNoMemory)
			{
			chunk.Decommit(commitEnd,KPageSize);
			commitEnd += KPageSize;
			}
		}
	while (r == KErrNoMemory);

	pos+= writePtr.Length();

	test.Printf(_L("Gsetting size of file ..."));
	r = f.Size(fileSize);
	test_KErrNone(r);
	test_Equal(fileSize,pos);

	test.Printf(_L("Closing file ..."));
	f.Close();

	test.Printf(_L("Closing chunk ..."));
	chunk.Close();

	test.Printf(_L("FreeRam = %d"), FreeRam());
	}



LOCAL_C void UnitTests()
//
// Test read file handling.
//
	{

	test.Start(_L("File cache read and write unit tests"));
	
//TheFs.SetDebugRegister(KCACHE);
	
	RFile f;
	TInt r;
	TInt pos;
	TInt len;
	TInt testNum;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TBool simulatelockFailureMode;
	TFileCacheStats fileCacheStats;
	r = controlIo(TheFs, gDrive, KControlIoFileCacheStats, fileCacheStats);
	test_KErrNone(r);
	test.Printf(_L("Number of files on closed queue=%d\n"),fileCacheStats.iFilesOnClosedQueue);
	test(fileCacheStats.iFilesOnClosedQueue == 0);
#endif

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// turn OFF lock failure mode 
	simulatelockFailureMode = EFalse;
	r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_KErrNone(r);
#endif

	TFileName testFile   = _L("TEST.BIN");

	//**********************************
	// Test Read-Modify-Write
	//**********************************
	test.Next(_L("Test read-modify-write"));
	gBufPtr.SetLength(KBufSize);
	FillBuffer(gBufPtr, KBufSize);
	TestBuffer(gBufPtr, 0,KBufSize);
	TPtrC8 writePtr;
	TPtr8 readPtr(gBuf->Des());
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TInt uncachedBytesRead;
	TInt uncachedPacketsRead;
#endif

	LowMemoryTest();

	// create an empty file, so that any writes overlapping segemt boundaries
	// need a read first
	// create a test file using directIO and then re-open it in buffered mode, 
	// so that any writes overlapping segemt boundaries need a read first
	r = f.Replace(TheFs, testFile, EFileWrite | EFileWriteDirectIO);
	test_KErrNone(r);
	writePtr.Set(gBuf->Des());
	r = f.Write(0, writePtr);
	test_KErrNone(r);
	f.Close();
	r = f.Open(TheFs, testFile, EFileReadBuffered | EFileWrite | EFileWriteBuffered);
	test_KErrNone(r);

	TInt cacheLineLen = 128*1024;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheStats(fileCacheStats);
	uncachedBytesRead = fileCacheStats.iUncachedBytesRead;
	uncachedPacketsRead = fileCacheStats.iUncachedPacketsRead;
#endif

	// write 1 partial segment at offset 0 to 7 in segment
	test.Next(_L("Test read-modify-write #1"));
	pos = cacheLineLen*0 + KSegmentSize*2 + 0;
	len = 7;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);
	// read back & verify whole segment
	pos&= ~KSegmentSizeMask; len = (len + KSegmentSize-1) & ~KSegmentSizeMask;
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	readPtr.FillZ();
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);
	test_KErrNone(r);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheStats(fileCacheStats);
	test (fileCacheStats.iUncachedBytesRead - uncachedBytesRead == KSegmentSize);
	test (fileCacheStats.iUncachedPacketsRead - uncachedPacketsRead == 1);
	uncachedBytesRead = fileCacheStats.iUncachedBytesRead;
	uncachedPacketsRead = fileCacheStats.iUncachedPacketsRead;
#endif

	// write 1 partial segment at offset 7 to 4096 in segment
	test.Next(_L("Test read-modify-write #2"));
	pos = cacheLineLen*0 + KSegmentSize*3 + 7;
	len = KSegmentSize - 7;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);
	// read back & verify whole segment
	pos&= ~KSegmentSizeMask; len = (len + KSegmentSize-1) & ~KSegmentSizeMask;
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	readPtr.FillZ();
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);
	test_KErrNone(r);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheStats(fileCacheStats);
	test (fileCacheStats.iUncachedBytesRead - uncachedBytesRead == KSegmentSize);
	test (fileCacheStats.iUncachedPacketsRead - uncachedPacketsRead == 1);
	uncachedBytesRead = fileCacheStats.iUncachedBytesRead;
	uncachedPacketsRead = fileCacheStats.iUncachedPacketsRead;
#endif

	// write 1 partial segment at offset 7 to 37 in segment
	test.Next(_L("Test read-modify-write #3"));
	pos = cacheLineLen*0 + KSegmentSize*4 + 7;
	len = 30;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);
	// read back & verify whole segment
	pos&= ~KSegmentSizeMask; len = (len + KSegmentSize-1) & ~KSegmentSizeMask;
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	readPtr.FillZ();
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);
	test_KErrNone(r);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheStats(fileCacheStats);
	test (fileCacheStats.iUncachedBytesRead - uncachedBytesRead == KSegmentSize);
	test (fileCacheStats.iUncachedPacketsRead - uncachedPacketsRead == 1);
	uncachedBytesRead = fileCacheStats.iUncachedBytesRead;
	uncachedPacketsRead = fileCacheStats.iUncachedPacketsRead;
#endif

	// write 2 segments, first and last both partial
	test.Next(_L("Test read-modify-write #4"));
	pos = cacheLineLen*1 + KSegmentSize*2 + 3;
	len = KSegmentSize * 1;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);
	// read back & verify whole segment
	pos&= ~KSegmentSizeMask; len = (len + KSegmentSize-1) & ~KSegmentSizeMask;
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	readPtr.FillZ();
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);
	test_KErrNone(r);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheStats(fileCacheStats);
	test (fileCacheStats.iUncachedBytesRead - uncachedBytesRead == KSegmentSize*2);
	// should read both segments in one read as they are contiguous
	test (fileCacheStats.iUncachedPacketsRead - uncachedPacketsRead == 1);	
	uncachedBytesRead = fileCacheStats.iUncachedBytesRead;
	uncachedPacketsRead = fileCacheStats.iUncachedPacketsRead;
#endif

	// write 3 segments, first and last both partial
	test.Next(_L("Test read-modify-write #5"));
	pos = cacheLineLen*2 + KSegmentSize*2 + 7;
	len = KSegmentSize * 2;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);
	// read back & verify whole segment
	pos&= ~KSegmentSizeMask; len = (len + KSegmentSize-1) & ~KSegmentSizeMask;
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	readPtr.FillZ();
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);
	test_KErrNone(r);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheStats(fileCacheStats);
	test (fileCacheStats.iUncachedBytesRead - uncachedBytesRead == KSegmentSize*2);
	test (fileCacheStats.iUncachedPacketsRead - uncachedPacketsRead == 2);
	uncachedBytesRead = fileCacheStats.iUncachedBytesRead;
	uncachedPacketsRead = fileCacheStats.iUncachedPacketsRead;
#endif

	f.Close();

	//**************************************************************
	// Test dirty data NOT written to disk if continuously writing to a file which fits in tke cache
	//**************************************************************
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	test.Printf(_L("Test dirty data NOT written to disk if continuously writing to a file which fits in tke cache...\n"));

	// flush closed files queue to empty cache 
	test.Printf(_L("Flushing close queue to empty cache...\n"));
	r = TheFs.ControlIo(gDrive, KControlIoFlushClosedFiles);
	test_KErrNone(r);

	r = f.Replace(TheFs, testFile, EFileReadBuffered | EFileWrite | EFileWriteBuffered);
	test_KErrNone(r);

	r = f.SetSize(gFileCacheConfig.iCacheSize);
	test_KErrNone(r);

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,gFileCacheConfig.iClosedFileKeepAliveTime*3); // write 3 times the flush timer

	pos = 0;
	TInt bufLen = 7;
	TInt maxLockedSegmentCount = 0;
	while (reqStat == KRequestPending)
		{
		bufLen = (bufLen + 1023) & 0x3FFF;
		len = Min(bufLen, KBufSize - pos);
		len = Min(len, gFileCacheConfig.iCacheSize-pos);
		TPtrC8 writePtr = gBuf->Mid(pos, len);
		r = f.Write(pos, writePtr, len);
		test_KErrNone(r);
		pos+= len;
		TInt r = controlIo(TheFs,gDrive, KControlIoFileCacheStats, fileCacheStats);
		test_KErrNone(r);
		// test the locked (i.e. dirty) count always goes up & never down (down would indicate a flush)
		if (fileCacheStats.iLockedSegmentCount != maxLockedSegmentCount)
			test.Printf(_L("iLockedSegmentCount %d...\n"), fileCacheStats.iLockedSegmentCount);

		test(fileCacheStats.iLockedSegmentCount >= maxLockedSegmentCount);
		maxLockedSegmentCount = Max(maxLockedSegmentCount, fileCacheStats.iLockedSegmentCount);
		// wrap to start of file
		if (pos >= gFileCacheConfig.iCacheSize || pos >= KBufSize)	
			pos = 0;
		}
	timer.Close();

	test(fileCacheStats.iLockedSegmentCount > 0);

	if (gDriveCacheFlags & (EFileCacheWriteEnabled | EFileCacheWriteOn))
		TestDirtyDataWrittenToDisk(fileCacheStats);
	f.Close();
#endif


	//**************************************************************
	// Test dirty data written to disk
	//**************************************************************
	enum {ETestDataWrittenAfterTimeout, ETestDataWrittenAfterFileClose, ETestDataWrittenAfterSessionClose, ETestEnd};
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	for (testNum = 0; testNum < ETestEnd; testNum++)
#else
	for (testNum = 0; testNum < 1; testNum++)
#endif
		{
		switch(testNum)
			{
			case ETestDataWrittenAfterTimeout		: test.Next(_L("ETestDataWrittenAfterTimeout"));		break;
			case ETestDataWrittenAfterFileClose		: test.Next(_L("ETestDataWrittenAfterFileClose"));		break;
			case ETestDataWrittenAfterSessionClose	: test.Next(_L("ETestDataWrittenAfterSessionClose"));	break;
			};

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// flush closed files queue to empty cache 
		test.Printf(_L("Flushing close queue to empty cache...\n"));
		r = TheFs.ControlIo(gDrive, KControlIoFlushClosedFiles);
		test_KErrNone(r);
#endif

		r = f.Replace(TheFs, testFile, EFileReadBuffered | EFileWrite | EFileWriteBuffered);
		test_KErrNone(r);


		gBufPtr.SetLength(KBufSize);
		
		test.Printf(_L("writing file in small blocks to test write caching...\n"));

		FillBuffer(gBufPtr, KBufSize);
		TestBuffer(gBufPtr, 0,KBufSize);

		pos = 0;
		TInt bufLen = 7;
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		TInt maxLockedSegmentCount = 0;
		TInt maxUsedCount = 0;
#endif
		while (pos < KBufSize) 
			{
			bufLen = (bufLen + 1023) & 0x3FFF;
			len = Min(bufLen, KBufSize - pos);
//RDebug::Print(_L("write len %d"), len);
			TPtrC8 writePtr = gBuf->Mid(pos, len);
			r = f.Write(pos, writePtr, len);
			test_KErrNone(r);
			pos+= len;
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
//			PrintFileCacheStats(fileCacheStats);
			TInt r = controlIo(TheFs,gDrive, KControlIoFileCacheStats, fileCacheStats);
			test_KErrNone(r);
			maxLockedSegmentCount = Max(maxLockedSegmentCount, fileCacheStats.iLockedSegmentCount);
			maxUsedCount = Max(maxUsedCount, fileCacheStats.iUsedCount);
#endif
			}
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		test.Next(_L("Test maxiimum locked page and cacheline count is reasonable"));
		test.Printf(_L("maxLockedSegmentCount %d maxUsedCount %d"), maxLockedSegmentCount, maxUsedCount);
		test (maxLockedSegmentCount > 0 && maxLockedSegmentCount <= (gFileCacheConfig.iCacheSize * 2) / KSegmentSize );
		test (maxUsedCount > 0 && maxUsedCount <= 5);
#endif



		if (testNum == ETestDataWrittenAfterTimeout)
			{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
			if (gDriveCacheFlags & (EFileCacheWriteEnabled | EFileCacheWriteOn))
				TestDirtyDataWrittenToDisk(fileCacheStats);
#endif
			f.Close();
			}

		if (testNum == ETestDataWrittenAfterFileClose)
			{
			f.Close();
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
			test.Next(_L("test dirty data has been written to disk after file close"));
			PrintFileCacheStats(fileCacheStats);
			test(fileCacheStats.iLockedSegmentCount == 0);
#endif
			}

		if (testNum == ETestDataWrittenAfterSessionClose)
			{
			// verify that closing the file server session (i.e. not the file)
			// flushes the data to disk...
			// *** NB This is likely to complete sometime AFTER returning from RFs::Close()
			// *** as  the file server doesn't wait for the disconnect thread to complete !!! 

			TheFs.Close();
			TheFs.Connect();
			SetSessionPath(gDrive);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
			test.Next(_L("Test dirty data is eventually written to disk after session close"));
			TestDirtyDataWrittenToDisk(fileCacheStats);
			test(fileCacheStats.iLockedSegmentCount == 0);
#endif
			}

		}	// for (TInt testNum = 0; testNum < ETestEnd; testNum++)


//#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
//	PrintFileCacheStats(fileCacheStats);
//#endif

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// flush closed files queue to empty cache 
	test.Printf(_L("Flushing close queue to empty cache...\n"));
	r = TheFs.ControlIo(gDrive, KControlIoFlushClosedFiles);
	test_KErrNone(r);
#endif

	r = f.Open(TheFs, testFile, EFileRead | EFileReadBuffered | EFileWrite);
	test_KErrNone(r);

	TInt size;
	r = f.Size(size);
	test_KErrNone(r);
	test (size = KBufSize);

	readPtr.Set(gBuf->Des());

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// Allocate full cachelines - so we can enable hole testing
	TBool allocateAllSegmentsInCacheLine = ETrue;
	r = controlIo(TheFs, gDrive, KControlIoAllocateMaxSegments, allocateAllSegmentsInCacheLine);
	test_KErrNone(r);
	PrintFileCacheStats(fileCacheStats, EFalse);
	TInt holesDetected = fileCacheStats.iHoleCount;
	TInt lockFailures = fileCacheStats.iCommitFailureCount + fileCacheStats.iLockFailureCount;
#endif

	test.Next(_L("Test reading a partially filled cacheline"));
	// create a cacheline with only the first segment filled
	TInt startPos = KSegmentSize;
	pos = startPos;
	len = KSegmentSize;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);

	// read from first & second segments
	pos = startPos;
	len = 8192;
	RDebug::Print(_L("+%x, %x\n"), pos, len);
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);

	
	test.Next(_L("Test reading an empty segment towards the end of a partially filled cacheline (hole test)"));
	// read from segment #4 and beyond end of cacheline into next
	pos = startPos + KSegmentSize * 4;
	len = KSegmentSize * 33;	// 132 K
	RDebug::Print(_L("+%x, %x\n"), pos, len);
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);


	test.Next(_L("Test writing to an empty segment towards the end of a partially filled cacheline (hole test)"));
	// create a cacheline with only the first segment filled
	startPos = 256 * 1024;			
	pos = startPos;
	len = KSegmentSize;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);


	// write into segment 3, 4 & 5
	pos = startPos + KSegmentSize * 2;
	len = KSegmentSize * 3;
	writePtr.Set(gBuf->Mid(pos, len));
	r = f.Write(pos, writePtr, len);
	test_KErrNone(r);
	// read back whole cacheline & verify
	pos = startPos;
	len = KSegmentSize * 32;	// 128 K
	RDebug::Print(_L("+%x, %x\n"), pos, len);
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheStats(fileCacheStats);
	if (fileCacheStats.iCommitFailureCount + fileCacheStats.iLockFailureCount != lockFailures)
		{
		if (gRomPaged)
			{
			test.Printf(_L("Lock failures detected on paged ROM, abandoning hole test"));
			}
		else
			{
			test.Printf(_L("Unexpected lock failures detected on unpaged ROM!!!"));
			test(0);
			}
		}
	else
		{
		test(fileCacheStats.iHoleCount > holesDetected);
		}
	// Don't allocate full cachelines any more
	allocateAllSegmentsInCacheLine = EFalse;
	r = controlIo(TheFs, gDrive, KControlIoAllocateMaxSegments, allocateAllSegmentsInCacheLine);
	test_KErrNone(r);
#endif



	gBufPtr.FillZ();
	gBufPtr.SetLength(KBufSize);

	

	readPtr.SetLength(0);


	// read from middle of fifth sector to half way thru seventh
	test.Next(_L("Test read that spans two pages"));
	pos = 512*4 + 16;
	len = 512*2;
	RDebug::Print(_L("+%x, %x\n"), pos, len);
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);



	// read to end of file
	test.Next(_L("Test reading past end of file"));
    pos = KBufSize - 0x100;			// 0xfb05;
	len = KBufSize - pos;
//	pos = KBufSize - 16;
//	len = 512;;
//	len = Min(len, KBufSize-pos);
	RDebug::Print(_L("+%x, %x\n"), pos, len);
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	r = f.Read(pos, readPtr, len/* + 0x67*/);
	test_KErrNone(r);

	test.Next(_L("Test async reads"));
	// issue 2 async reads
	pos = KSegmentSize*7 + 16;
	len = KSegmentSize;
	RDebug::Print(_L("+%x, %x\n"), pos, len);
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);

	TInt pos2 = pos + len;
	TInt len2 = KSegmentSize;
	TPtr8 readPtr2 = gBuf->Des();
	readPtr2.Set(gBufPtr.MidTPtr(pos2, len2));
	readPtr2.SetLength(len2);
	
	TRequestStatus status1(KRequestPending);
	TRequestStatus status2(KRequestPending);
	
	f.Read(pos, readPtr, len, status1);
	f.Read(readPtr2, len2, status2);


	User::WaitForRequest(status1);
	User::WaitForRequest(status2);
	test.Printf(_L("status1 %d status2 %d\n"), status1.Int(), status2.Int());
	TestBuffer(gBufPtr, pos, len);
	TestBuffer(gBufPtr, pos2, len2);


	test.Next(_L("Read entire file"));

	for (pos = 0, len = 1; len <= 1024 && pos < KBufSize; len = (len+1) & 0x3FF)
		{
		len = Min(len, KBufSize-pos);
//		RDebug::Print(_L("+%x, %x\n"), pos, len);
		readPtr.Set(gBufPtr.MidTPtr(pos, len));
		readPtr.SetLength(len);

		r = f.Read(pos, readPtr, len);
		test_KErrNone(r);
		TestBuffer(gBufPtr, pos, len);
		test_KErrNone(r);
		pos+= len;
		}
	
	TestBuffer(gBufPtr, 0, KBufSize);

	// read from position zero to ensure it's cached
	pos = 0;
	len = 512;
	readPtr.Set(gBufPtr.MidTPtr(pos, len));
	readPtr.SetLength(len);
	r = f.Read(pos, readPtr, len);
	test_KErrNone(r);
	TestBuffer(gBufPtr, pos, len);
	test_KErrNone(r);

	f.Close();


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	r = controlIo(TheFs, gDrive, KControlIoFileCacheStats, fileCacheStats);
	test_KErrNone(r);
	test.Printf(_L("Number of files on closed queue=%d\n"),fileCacheStats.iFilesOnClosedQueue);
	test(fileCacheStats.iFilesOnClosedQueue == 1);
#endif
	//

	
	//**************************************************************
	// Test closed file queue 
	//**************************************************************
	enum 
		{
		ETestCloseQueueEmptyAfterTimeout, 
		ETestCloseQueueEmptyAfterOpenWithDirectIo, 
		ETestCloseQueueEmptyAfterDelete, 
		ETestCloseEnd};
	for (testNum = 0; testNum < ETestCloseEnd; testNum++)
		{
	
		test.Next(_L("Reopen file & verify closed queue is empty"));

		r = f.Open(TheFs, testFile, EFileRead | EFileReadBuffered);
		test_KErrNone(r);
		pos = 0;
		len = 512;
		readPtr.Set(gBufPtr.MidTPtr(pos, len));
		readPtr.SetLength(len);
		r = f.Read(pos, readPtr, len);
		test_KErrNone(r);
		TestBuffer(gBufPtr, pos, len);
		test_KErrNone(r);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		r = controlIo(TheFs, gDrive, KControlIoFileCacheStats, fileCacheStats);
		test_KErrNone(r);
		test.Printf(_L("Number of files on closed queue=%d\n"),fileCacheStats.iFilesOnClosedQueue);
		test(fileCacheStats.iFilesOnClosedQueue == 0);
#endif
		f.Close();

		test.Next(_L("close & verify file is back on close queue"));


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		r = controlIo(TheFs, gDrive, KControlIoFileCacheStats, fileCacheStats);
		test_KErrNone(r);
		test.Printf(_L("Number of files on closed queue=%d\n"),fileCacheStats.iFilesOnClosedQueue);
		test(fileCacheStats.iFilesOnClosedQueue == 1);
#endif

		if (testNum == ETestCloseQueueEmptyAfterDelete)
			{
			test.Next(_L("delete file and verify closed queue is empty"));
			r = TheFs.Delete(testFile);
			test_KErrNone(r);
			}
		if (testNum == ETestCloseQueueEmptyAfterTimeout)
			{
			test.Next(_L("wait for a while and verify closed queue is empty"));
			// wait for closed file queue to empty
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
			User::After(gFileCacheConfig.iClosedFileKeepAliveTime + 1000000);
#endif
			}
		if (testNum == ETestCloseQueueEmptyAfterOpenWithDirectIo)
			{
			test.Next(_L("Re-open file in directIo mode and then close and verify closed queue is empty"));
			r = f.Open(TheFs, testFile, EFileRead | EFileReadDirectIO);
			test_KErrNone(r);
			f.Close();
			}

	#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		r = controlIo(TheFs, gDrive, KControlIoFileCacheStats, fileCacheStats);
		test_KErrNone(r);
		test.Printf(_L("Number of files on closed queue=%d\n"),fileCacheStats.iFilesOnClosedQueue);
		test(fileCacheStats.iFilesOnClosedQueue == 0);
	#endif
		}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// turn lock failure mode back ON (if enabled)
	simulatelockFailureMode = ETrue;
	r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_KErrNone(r);
#endif

	//**************************************************************
	// Test opening a file with a silly open mode flags combinations 
	//**************************************************************
	test.Next(_L("Open file with illegal cache on/off bits"));
	r = f.Open(TheFs, testFile, EFileRead | EFileReadBuffered | EFileReadDirectIO);
	test_Value(r, r==KErrArgument);
	r = f.Open(TheFs, testFile, EFileRead | EFileWriteBuffered | EFileWriteDirectIO);
	test_Value(r, r==KErrArgument);
	//**********************************
	// Test that continuously appending to a file yields the correct size...
	// NB: Must have lock failure mode ON in debug mode for this test to pass
	//**********************************
	test.Next(_L("Test appending to a file & checking the file size..."));
	gBufPtr.SetLength(KBufSize);

	const TInt KWriteLen = KSegmentSize+1;
	writePtr.Set(gBuf->Des().Ptr(), KWriteLen);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	r = controlIo(TheFs, gDrive, KControlIoFileCacheStats, fileCacheStats);
	test_KErrNone(r);
	test.Printf(_L("Number of Write-throughs with dirty data=%d\n"),fileCacheStats.iWriteThroughWithDirtyDataCount);
	TInt writeThroughWithDirtyDataCountOld = fileCacheStats.iWriteThroughWithDirtyDataCount;
	TInt writeThroughWithDirtyDataCountNew = writeThroughWithDirtyDataCountOld;
#endif

	TInt fileSize = 0;
	for (TInt i=0; i<4; i++)
		{
		r = f.Replace(TheFs, testFile, EFileWrite | EFileWriteBuffered);
		test_KErrNone(r);

		fileSize = 0;
		r = f.SetSize(fileSize);
		test_KErrNone(r);
		for (pos = 0; pos < KMaxFileSize; )
			{
			r = f.Write(pos, writePtr);
			if (r != KErrNone)
				{
				test.Printf(_L("Iter #%d, write pos %d size %d, Write() returned %d"), i, pos, fileSize, r);
				r = f.Flush();
				test.Printf(_L("Flush returned %d"), r);
				test(0);
				}
			test_KErrNone(r);
			pos+= writePtr.Length();

			r = f.Size(fileSize);
			test_KErrNone(r);
			if (fileSize != pos)
				{
				test.Printf(_L("Iter #%d, write pos %d != size %d"), i, pos, fileSize);
				r = f.Flush();
				test.Printf(_L("Flush returned %d"), r);
				test(0);
				}
			test (fileSize == pos);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
			r = controlIo(TheFs, gDrive, KControlIoFileCacheStats, fileCacheStats);
			test_KErrNone(r);
			writeThroughWithDirtyDataCountNew = fileCacheStats.iWriteThroughWithDirtyDataCount;
			if (writeThroughWithDirtyDataCountNew > writeThroughWithDirtyDataCountOld)
				{
				test.Printf(_L("Iter #%d, write pos %d size %d"), i, pos, fileSize);
				test.Printf(_L("Number of Write-throughs with dirty data=%d\n"),fileCacheStats.iWriteThroughWithDirtyDataCount);
				i = 4;
				break;
				}
#endif

			}
		// Close & delete the file after filling it - to ensure all pages are decomitted
		// otherwise we may never again get a simulated lock failure
		r = f.Flush();
		test_KErrNone(r);
		f.Close();
		r = TheFs.Delete(testFile);
		test_KErrNone(r);
		}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	test(writeThroughWithDirtyDataCountNew > writeThroughWithDirtyDataCountOld);
#endif


	f.Close();


//TheFs.SetDebugRegister(0);
	test.End();
	}


// This thread is used to test what happens if requests are sent to the file server
// while the drive thread is hung in the critical notifier server. The requests should
// complete immediately with KErrNotReady
TInt ReaderThread(TAny* aFileName)
	{
	RTest test(_L("T_FILECACHE, ReaderThread"));
	
	TDesC& fileName = *(TDesC*) aFileName;

	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	r = fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);


	RFile file;
	r = file.Open(fs, fileName, EFileRead | EFileReadBuffered | EFileShareReadersOrWriters);
	test_KErrNone(r);

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,10000000); // Read for 10 secs

	TInt pos=0;
	TInt KReadLen = 32768;
	TInt retCode = KErrNone;
		
	for (TInt i=0; reqStat == KRequestPending; i++)
		{
		r = file.Read(pos, DataBuf, KReadLen);
		test_Value(r, r == KErrNone || r == KErrNotReady);

		//test.Printf(_L("ReaderThread: iter %d, read at %d ret %d\n"), i, pos, r);
		if (r == KErrNotReady)
			retCode = r;
		else if (r == KErrNone)
			pos = DataBuf.Length() == 0 ? 0: pos + DataBuf.Length();
		if (retCode == KErrNotReady)
			test.Printf(_L("ReaderThread: iter %d, read at %d ret %d \n"), i, pos, r);
		}

	timer.Close();
	file.Close();
	fs.Close();

	return retCode;
	}


void ManualTests()
	{
	RFile f;
	TInt r;
	gBufPtr.SetLength(KBufSize);
	FillBuffer(gBufPtr, KBufSize);
	TestBuffer(gBufPtr, 0,KBufSize);

	TPtrC8 writePtr;
	writePtr.Set(gBuf->Des());
	
	TFileName testFile   = _L("TEST.BIN");
	
#if defined(_DEBUG)
	test.Next(_L("Testing writing and then closing with an immediate simulated card eject"));
	test.Printf(_L("**********************************************************\n"));
	test.Printf(_L("When critical notifier message appears, PRESS RETRY\n"));
	test.Printf(_L("**********************************************************\n"));

	r = f.Replace(TheFs, testFile, EFileWrite | EFileWriteBuffered | EFileShareReadersOrWriters);
	test_KErrNone(r);

	test.Printf(_L("Writing...."));
	r = f.Write(0, writePtr);
	test.Printf(_L("Write returned %d\n"), r);
	test_KErrNone(r);

	r=TheFs.ControlIo(gDrive, KControlIoSimulateFileCacheWriteFailure);
	test_KErrNone(r);

	f.Close();

	TEntry entry;

	// wait for re-insertion....
	do
		{
		r = TheFs.Entry(testFile, entry);
		User::After(500000);
		}
	while (r == KErrNotReady);
	test_KErrNone(r);

	test.Printf(_L("FileSize %d expected %d\n"), entry.iSize, writePtr.Length());

	// test all data is written to disk 
	test (entry.iSize == writePtr.Length());
#endif


	test.Next(_L("Testing writing and then ejecting card"));

//	r = f.Replace(TheFs, testFile, EFileWrite | EFileWriteDirectIO);
	r = f.Replace(TheFs, testFile, EFileWrite | EFileWriteBuffered | EFileShareReadersOrWriters);
	test_KErrNone(r);
	writePtr.Set(gBuf->Des());


	// set the file size first so that the cluster chain already exists
	r = f.SetSize(writePtr.Length());
	test.Printf(_L("SetSize returned %d"));

	// Create another thread which will attempt to issue reads while this thread is suspended
	// These reads should complete with KErrNotReady
	const TInt KHeapSize = 32768;
	test.Printf(_L("Creating file-reading thread\n"));
	RThread readerThread;
	readerThread.Create(_L("FileReaderThread"), ReaderThread, KDefaultStackSize, KHeapSize, KHeapSize, &testFile);

	test.Printf(_L("Writing...."));
	r = f.Write(0, writePtr);
	test.Printf(_L("Write returned %d\n"), r);


	test.Printf(_L("Waiting 5 seconds.\n"));
	test.Printf(_L("**********************************************************\n"));
	test.Printf(_L("Please press a key, EJECT MEDIA BEFORE WRITE TIMER EXPIRES,\n"));
	test.Printf(_L("wait for critical notifier, replace media and retry\n"));
	test.Printf(_L("**********************************************************\n"));
//	test.Printf(_L("press any key..."));
	test.Getch();
	test.Printf(_L("\n"));

	test.Printf(_L("Writing...."));
	r = f.Write(0, writePtr);
	test.Printf(_L("Write returned %d\n"), r);
	readerThread.Resume();

	User::After(5000000);

	do
		{
		test.Printf(_L("Flushing....\n"));
		r = f.Flush();
		test.Printf(_L("Flush returned %d\n"), r);
		if (r != KErrNone)
			User::After(5000000);
		}
	while (r != KErrNone);


//	r = f.Write(0, writePtr);
//	test.Printf(_L("Write returned %d"));

//	test_KErrNone(r);

	test.Next(_L("Testing reader thread completed with KErrNotReady"));
	TRequestStatus status;
	readerThread.Logon(status);
	readerThread.Resume();
	User::WaitForRequest(status);
	test.Printf(_L("ReaderThread status  %d\n"), status.Int());
	test (status.Int() == KErrNotReady);
	readerThread.Close();

	test.Printf(_L("press any key..."));
	test.Getch();
	test.Printf(_L("\n"));

	f.Close();
	}


LOCAL_C void DoTestFileRead(TUint aFileMode, TInt aReadBlockSize)
//
// Do Read Test
//
	{

	RFile file;
	TInt r=file.Open(TheFs,_L("READTEST.XXX"),EFileStream | aFileMode);
	test_KErrNone(r);

	TInt maxReadCount=KMaxFileSize/aReadBlockSize;
	TInt loopCount=0;

	TTime startTime;
	TTime endTime;

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,10000000); // After 10 secs
	startTime.HomeTime();

	FOREVER
		{
		TInt pos=0;
		file.Seek(ESeekStart,pos);
		for(TInt ii=0;ii<maxReadCount;ii++)
			{
			r = file.Read(DataBuf,aReadBlockSize);
			test_KErrNone(r);
#if defined(_DEBUG)
			for(TInt jj=0;jj<DataBuf.Size();jj++)
				{
				if (DataBuf[jj] != ((jj+ii*aReadBlockSize)%256))
					{
					test.Printf(_L("len %d size %d jj %d ii %d"), DataBuf.Length(), DataBuf.Size(), jj, ii);
					test(0);
					}
//				test(DataBuf[jj] == ((jj+ii*aReadBlockSize)%256) );
				}
#endif
			if (reqStat!=KRequestPending)
				{
				endTime.HomeTime();

				TInt functionCalls=loopCount*maxReadCount+ii;
//				TReal rate = ( TReal32(functionCalls) * TReal32(aReadBlockSize) ) / 10;
//				test.Printf(_L("Read %5d byte blocks:\t%11.3f KBytes/s\n"), aReadBlockSize, rate / (KOneK));
				
				TReal transferRate = 
					(TReal32(functionCalls) * TReal32(aReadBlockSize)) / 
					TReal(endTime.MicroSecondsFrom(startTime).Int64()) * TReal(KOneMeg) / TReal(KOneK);
				test.Printf(_L("Read %5d byte blocks:\t%11.3f KBytes/s\n"), aReadBlockSize,transferRate);

				timer.Close();
				file.Close();

				return;
				}
			}
		loopCount++;
		}
	}

LOCAL_C void DoTestFileWrite(TUint aFileMode, TInt aWriteBlockSize)
//
// Do Write benchmark
//
	{
	DataBuf.SetLength(aWriteBlockSize);
	TInt maxWriteCount=KMaxFileSize/aWriteBlockSize;
	TInt loopCount=0;

	RFile file;
	TInt r = file.Replace(TheFs,_L("WRITETST"),EFileStream | aFileMode);
	test_KErrNone(r);

	TTime startTime;
	TTime endTime;

	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,10000000); // After 10 secs
	startTime.HomeTime();

	

	FOREVER
		{
		TInt pos=0;
		file.Seek(ESeekStart,pos);
		for(TInt ii=0;ii<maxWriteCount;ii++)
			{
			r = file.Write(DataBuf);
			test_KErrNone(r);
			if (reqStat!=KRequestPending)
				{
				
//				TReal rate = (TReal32(functionCalls) * TReal32(aWriteBlockSize)) / 10;
//				test.Printf(_L("Write %5d byte blocks:\t%11.3f KBytes/s\n"), aWriteBlockSize, rate / (KOneK));

				r = file.Flush();
				test_KErrNone(r);

				endTime.HomeTime();

				TInt functionCalls=loopCount*maxWriteCount+ii;
				TReal transferRate = 
					(TReal32(functionCalls) * TReal32(aWriteBlockSize)) / 
					TReal(endTime.MicroSecondsFrom(startTime).Int64()) * TReal(KOneMeg) / TReal(KOneK);
				test.Printf(_L("Write %5d byte blocks:\t%11.3f KBytes/s\n"), aWriteBlockSize,transferRate);

				file.Close();
				timer.Close();
				TInt r=TheFs.Delete(_L("WRITETST"));
				test_KErrNone(r);
				return;
				}
			}
		loopCount++;
		}
	}

LOCAL_C void TestFileRead(TUint aFileMode)
//
// Benchmark read method
//
	{

//	ClearSessionDirectory();
	test.Next(_L("Benchmark read method"));
//	test.Printf(_L("aFileMode %08X\n"), aFileMode);
	PrintFileMode(aFileMode);

// Create test data
	TBuf8<1024> testdata(1024);
	for (TInt i=0;i<testdata.MaxSize();i++)
		testdata[i]=TText8(i%256);		
	
	// create & fill the file
	RFile file;
	TInt r=file.Replace(TheFs,_L("READTEST.XXX"),EFileStream | aFileMode);
	test_KErrNone(r);

	test.Printf(_L("Creating test file....."));
	TInt count=KMaxFileSize/testdata.Length();
	while (count--)
		file.Write(testdata);
	file.Close();
	test.Printf(_L("done.\n"));

#ifdef SYMBIAN_TEST_EXTENDED_BUFFER_SIZES

	DoTestFileRead(aFileMode, 1);
	DoTestFileRead(aFileMode, 2);
	DoTestFileRead(aFileMode, 4);
	DoTestFileRead(aFileMode, 8);
	DoTestFileRead(aFileMode, 16);
	DoTestFileRead(aFileMode, 32);
	DoTestFileRead(aFileMode, 64);
	DoTestFileRead(aFileMode, 128);
	DoTestFileRead(aFileMode, 256);
	DoTestFileRead(aFileMode, 512);
	DoTestFileRead(aFileMode, 1024);
	DoTestFileRead(aFileMode, 2048);
	DoTestFileRead(aFileMode, 4096);
	DoTestFileRead(aFileMode, 8192);
	DoTestFileRead(aFileMode, 16384);
	DoTestFileRead(aFileMode, 32768);
	DoTestFileRead(aFileMode, 65536);
	DoTestFileRead(aFileMode, 131072);
#else
//TheFs.SetDebugRegister(KCACHE);
	DoTestFileRead(aFileMode, 1);
	DoTestFileRead(aFileMode, 16);
	DoTestFileRead(aFileMode, 512);
//TheFs.SetDebugRegister(0);
	DoTestFileRead(aFileMode, 4 * 1024);
	DoTestFileRead(aFileMode, 32 * 1024);
	DoTestFileRead(aFileMode, 64 * 1024);
	DoTestFileRead(aFileMode, 128 * 1024);
	DoTestFileRead(aFileMode, 256 * 1024);
#endif

	r=TheFs.Delete(_L("READTEST.XXX"));



	test_KErrNone(r);
	}


LOCAL_C void TestFileWrite(TUint aFileMode)
//
// Benchmark write method
//
	{
//	ClearSessionDirectory();
	test.Next(_L("Benchmark write method"));
//	test.Printf(_L("aFileMode %08X\n"), aFileMode);
	PrintFileMode(aFileMode);

	
//	RFile file;
//	TInt r = file.Replace(TheFs,_L("WRITETST"),EFileStream | aFileMode);
//	test_KErrNone(r);


#ifdef SYMBIAN_TEST_EXTENDED_BUFFER_SIZES
	DoTestFileWrite(aFileMode, 1);
	DoTestFileWrite(aFileMode, 2);
	DoTestFileWrite(aFileMode, 4);
	DoTestFileWrite(aFileMode, 8);
	DoTestFileWrite(aFileMode, 16);
	DoTestFileWrite(aFileMode, 32);
	DoTestFileWrite(aFileMode, 64);
	DoTestFileWrite(aFileMode, 128);
	DoTestFileWrite(aFileMode, 256);
	DoTestFileWrite(aFileMode, 512);
	DoTestFileWrite(aFileMode, 1024);
	DoTestFileWrite(aFileMode, 2048);
	DoTestFileWrite(aFileMode, 4096);
	DoTestFileWrite(aFileMode, 8192);
	DoTestFileWrite(aFileMode, 16384);
	DoTestFileWrite(aFileMode, 32768);
	DoTestFileWrite(aFileMode, 65536);
	DoTestFileWrite(aFileMode, 131072);
#else
	DoTestFileWrite(aFileMode, 1);
	DoTestFileWrite(aFileMode, 16);
	DoTestFileWrite(aFileMode, 512);
	DoTestFileWrite(aFileMode, 4 * 1024);
	DoTestFileWrite(aFileMode, 32 * 1024);
	DoTestFileWrite(aFileMode, 64 * 1024);
	DoTestFileWrite(aFileMode, 128 * 1024);
	DoTestFileWrite(aFileMode, 256 * 1024);
#endif
	}

enum CommandId {
    CMD_NO_COMMAND,
    CMD_HELP,
	CMD_DISPLAY_CACHE_FLAGS,
	CMD_WRITE_CACHE_FLAGS,
	CMD_PERFORMANCE_TEST,
	CMD_MANUAL_TEST
};


class CommandLineOption { 
public:
    CommandLineOption(const TPtrC &s, CommandId i) : str(s), id(i) { }
    
    const TPtrC str;
    CommandId   id;
    
private:
    CommandLineOption() { }
};

// Lists of command line options
static const CommandLineOption commandList[] = {
    CommandLineOption(_L("-help"), CMD_HELP),
    CommandLineOption(_L("-h"), CMD_HELP),
    CommandLineOption(_L("-?"), CMD_HELP),
    CommandLineOption(_L("/?"), CMD_HELP),
    CommandLineOption(_L("-d"), CMD_DISPLAY_CACHE_FLAGS),
    CommandLineOption(_L("-w"), CMD_WRITE_CACHE_FLAGS),
    CommandLineOption(_L("-p"), CMD_PERFORMANCE_TEST),
    CommandLineOption(_L("-m"), CMD_MANUAL_TEST),
};



LOCAL_C void printHelp() {
    test.Printf(_L("Option          Explanation\r\n"));
    test.Printf(_L("-help           Print this text\r\n"));
    test.Printf(_L("-d				Display cache flags\n"));
    test.Printf(_L("-p				Performance test\n"));
}



/////////////////////////////////////////////////////////////////////////////
// Function: 
//		bool parseCommandLine()
//
// Parameters:
//		-
//
// Return value:
//		true if command line options was correct, false if any command was
//      invalid
//
// Purpose:
//		Parses the command line
//
/////////////////////////////////////////////////////////////////////////////

//TBuf<512> commandLine;


LOCAL_C bool ParseCommandLine( const TDesC& aCommand )
	{
    TLex lex(aCommand);
    TInt tokenCount = 0;

	gDriveToTest='C';		
	gRunTests = ETrue;

    for (TPtrC token=lex.NextToken(); token.Length() != 0;token.Set(lex.NextToken()))
		{
        tokenCount++;

        //
        // Search the list of commands for a match.
        //
        CommandId commandId = CMD_NO_COMMAND;
        for (TUint i = 0; i < sizeof commandList / sizeof commandList[0]; i++) 
			{
            if (token.CompareF(commandList[i].str) == 0) 
				{
                //
                // Found a matching string
                //
                commandId = commandList[i].id;
                break;
				}
			}

        switch (commandId) 
			{
			case CMD_NO_COMMAND:
				{
				TFileName thisfile=RProcess().FileName();
				if (token.MatchF(thisfile)==0)
					{
					token.Set(lex.NextToken());
					}
				test.Printf(_L("CLP=%S\n"),&token);

				TChar ch = token[0];
				if (ch.IsAlpha())
					{
					if(token.Length()!=0)		
						{
						gDriveToTest=token[0];
						gDriveToTest.UpperCase();
						}
					else						
						gDriveToTest='C';		
					lex.NextToken();
					}
				}
				break;

			case CMD_HELP:
				printHelp();
				gRunTests = EFalse;
				break;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
			case CMD_WRITE_CACHE_FLAGS:
				{         
					token.Set(lex.NextToken());
					if (token.Length() == 0) {
						printHelp();
						return false;
					}
					//
					// Extract flags (in hex) from next token.
					//
					TPtrC numStr(token);
					TUint val;
					TInt r = TLex(numStr).Val(val, EHex);
					if (r != KErrNone) {
						printHelp();
						return false;
					}
					gDriveCacheFlags = TFileCacheFlags(val);
					gWriteCacheFlags = ETrue;
					gRunTests = EFalse;
					break;
				}

			case CMD_DISPLAY_CACHE_FLAGS:
				gDisplayCacheFlags = ETrue;
				gRunTests = EFalse;
				break;
#endif
			case CMD_PERFORMANCE_TEST:
				gRunPerformanceTests = ETrue;
				gRunUnitTests = EFalse;
				break;

			case CMD_MANUAL_TEST:
				gRunManualTests = ETrue;
				gRunUnitTests = EFalse;
				break;

			default:
				test.Printf(_L("Sorry, command option '%S' not implemented\n"),
					&token);
				break;
			}

		}
    return true;
}


LOCAL_C TBool parseCommandLine() {
    //
    // Loop through all tokens in the command line
    //
	TInt cmdLineLen = User::CommandLineLength();
	HBufC* cmdLineBuf = HBufC::New( cmdLineLen );
    if( !cmdLineBuf )
		{
		return false;
		}
	TPtr ptr = cmdLineBuf->Des();
	User::CommandLine(ptr);

	bool err = ParseCommandLine( *cmdLineBuf );
	delete cmdLineBuf;
	return err;
	}

GLDEF_C void CallTestsL()
//
// Do tests relative to the session path
//
	{
	TestsInit();

	TVolumeInfo volInfo;
	TInt r = TheFs.Volume(volInfo, gDrive);
	test_KErrNone(r);

	TFullName extName;
	r = TheFs.ExtensionName(extName,gDrive, 0);
	if (r == KErrNone)
		{
		test.Printf(_L("File system extension present (%S)\n"), &extName);
		}

	if ((volInfo.iDrive.iType == EMediaRam) ||
		((gDriveCacheFlags & (EFileCacheReadEnabled | EFileCacheReadOn)) == 0))
		{
		if (gRunPerformanceTests)
			{
			TestFileRead(EFileReadDirectIO);
			TestFileWrite(EFileWriteDirectIO);
			}
		TestsEnd();
		return;
		}

	if (gRunUnitTests)
		UnitTests();

	if (gRunManualTests)
		{		
		ManualTests();
		}

	if (gRunPerformanceTests)
		{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// turn OFF lock failure mode
		TBool simulatelockFailureMode = EFalse;
		r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
		test_KErrNone(r);
#endif

		TestFileRead(EFileReadDirectIO);
		if (gDriveCacheFlags & (EFileCacheReadEnabled | EFileCacheReadOn))
			{
			TestFileRead(EFileReadBuffered | EFileReadAheadOff);
			TestFileRead(EFileReadBuffered | EFileReadAheadOn);
			}

		TestFileWrite(EFileWriteDirectIO);

		if (gDriveCacheFlags & (EFileCacheWriteEnabled | EFileCacheWriteOn))
			TestFileWrite(EFileWriteBuffered);


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// turn lock failure mode back ON (if enabled)
		simulatelockFailureMode = ETrue;
		r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
		test_KErrNone(r);
#endif
		}	// if (gRunPerformanceTests)

	TestsEnd();
	}



LOCAL_C void DoTests(TInt aDrive)
//
// Do testing on aDrive
//
	{

	SetSessionPath(aDrive);

// !!! Disable platform security tests until we get the new APIs
//	if(User::Capability() & KCapabilityRoot)
//		CheckMountLFFS(TheFs,driveLetter);
	
	User::After(1000000);

	TInt r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	TheFs.ResourceCountMarkStart();

	TRAP(r,CallTestsL());
	test_KErrNone(r);

	TheFs.ResourceCountMarkEnd();
	}


void Format(TInt aDrive)
//
// Format current drive
//
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,driveBuf,EQuickFormat,count);
	//TInt r=format.Open(TheFs,driveBuf,EFullFormat,count);
	test.Printf(_L("RFormat::Open() returned %d\n"), r);
	test_KErrNone(r);
	while(count)
		{
		TInt r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}

GLDEF_C TInt E32Main()
//
// Test with drive nearly full
//
	{

	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	__UHEAP_MARK;

	TBool parseOk = parseCommandLine();
	if (!parseOk)
		User::Leave(KErrNotSupported);


	TInt r = TheFs.Connect();
	test_KErrNone(r);

	r=TheFs.CharToDrive(gDriveToTest,gDrive);
	test_KErrNone(r);

#if !defined(__WINS__)
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	test.Start(_L("Check that the rom is paged"));
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	if (romHeader->iPageableRomStart != NULL)
		{
		test.Printf(_L("ROM is paged\n"));
		gRomPaged = ETrue;
		}
#endif
#endif

	// Get the TFileCacheFlags for this drive
	r = TheFs.Volume(gVolInfo, gDrive);
	if (r == KErrNotReady)
		{
		TDriveInfo info;
		TInt err = TheFs.Drive(info,gDrive);
		test_KErrNone(err);
		if (info.iType == EMediaNotPresent)
			test.Printf(_L("%c: Medium not present - cannot perform test.\n"), (TUint)gDriveToTest);
		else
			test.Printf(_L("medium found (type %d) but drive %c: not ready\nPrevious test may have hung; else, check hardware.\n"), (TInt)info.iType, (TUint)gDriveToTest);
		}
	else if (r == KErrCorrupt)
		{
		test.Printf(_L("%c: Media corruption; previous test may have aborted; else, check hardware\n"), (TUint)gDriveToTest);
		}
	test_KErrNone(r);
	gDriveCacheFlags = gVolInfo.iFileCacheFlags;
	test.Printf(_L("DriveCacheFlags for drive %C = %08X\n"), (TInt) gDriveToTest, gDriveCacheFlags);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	PrintFileCacheConfig(gFileCacheConfig, gDrive);
	
	if (gDisplayCacheFlags)
		{
		test.Printf(_L("Press any key...\n"));
		test.Getch();
		}

	if (gWriteCacheFlags)
		{
		test.Printf(_L("Writing DriveCacheFlags for drive %C = %08X\n"), (TInt) gDriveToTest, gDriveCacheFlags);
		r = controlIo(TheFs,gDrive, KControlIoFileCacheFlagsWrite, gDriveCacheFlags);
		test_KErrNone(r);
		}
#endif

	if (gRunTests)
		{
		test.Title();


		test.Start(_L("Starting tests..."));

		if ((gVolInfo.iDrive.iMediaAtt & KMediaAttFormattable))
			Format(gDrive);

//TheFs.SetDebugRegister(KCACHE);
			DoTests(gDrive);
//TheFs.SetDebugRegister(0);

		if ((gVolInfo.iDrive.iMediaAtt & KMediaAttFormattable))
			Format(gDrive);

			test.End();
//			}
		}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TFileCacheStats fileCacheStats;
	PrintFileCacheStats(fileCacheStats);
#endif

	TheFs.Close();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }
