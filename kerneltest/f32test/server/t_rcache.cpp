// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_rcache.cpp
// 
//

/**
 @file
 @internalTechnology 
*/
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32const.h>
#include <e32const_private.h>
#include <f32dbg.h>
#include "t_server.h"
#include <e32twin.h>
#include <e32rom.h>
#include <hal.h>
#include <u32hal.h>

const TInt KTotalCacheSize = 32 * 1024 * 1024;
const TInt KDefaultCacheSize = (128 + 12) * 1024;
const TInt KFilesNeededToFillCache = (KTotalCacheSize / KDefaultCacheSize) + 2;


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_RCACHE-0190
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ914
//! @SYMTestCaseDesc    This test case is exercising the Read Caching functionality added to
//!						the File Server. There are negative and positive tests.
//! @SYMTestActions     0   setup the environment to execute the tests
//!						1	TestNegative creates situations where the cached content needs to be
//!							flushed or removed  from the cache by corrupting files and verifies
//!							the cache behaviour
//!						2 	TestSimpleRead ensures that the cache is working in the simple cases,
//!							with a combination of sync and async reads:
//!								a. The file fits in the cache
//!								b. The file doesn't fit in the cache
//!						3 	TestRepeatedRead verifies the cache behaviour when a file is read an
//!							arbitrary number of times, with and without other operations ongoing
//!						4	TestReadAhead reads 3 times from a file and verifies that the read
//!							ahead functionality acts afterwards.
//!						5	TestConcurrent reads files concurrently and verifies how the cache
//!							copes with it.
//!						6 	TestFillCache fills the cache and then executes TestSimpleRead.
//!
//! @SYMTestExpectedResults finishes if the read cache behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------


////////////////////////////////////////////////////////////
// Template functions encapsulating ControlIo magic
//
GLDEF_D template <class C>
GLDEF_C TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
}

GLDEF_D RTest test(_L("T_RCACHE"));

GLDEF_D	RFs TheFs;
GLDEF_D TInt gDrive;
GLDEF_D TFileName gSessionPath;
GLDEF_D TChar gDriveToTest;
TThreadId gMainThreadId;

LOCAL_D HBufC8* gBuf = NULL;
LOCAL_D TPtr8 gBufReadPtr(NULL, 0);
LOCAL_D HBufC8* gBufSec = NULL;
LOCAL_D TPtr8 gBufWritePtr(NULL, 0);

LOCAL_D const TInt KOneK = 1024;
LOCAL_D const TInt KOneMeg = KOneK * 1024;
LOCAL_D const TInt KBlockSize = KOneK;
LOCAL_D const TInt KWaitRequestsTableSize = 256;

LOCAL_D TInt gSecondFileSize = 0;
LOCAL_D TInt gFirstFileSize = 0;
LOCAL_D TInt gCurrentFileSize = 0;

LOCAL_D TInt64 gMediaSize = 0;

LOCAL_D TTimeIntervalMicroSeconds gTimeTakenBigFile(0);
LOCAL_D TBuf16<25> gFirstFile;
LOCAL_D TBuf16<25> gSecondFile;
LOCAL_D TBuf16<25> gCurrentFile;

LOCAL_D TInt gNextFile = 0;
LOCAL_D TTime gTime1;
LOCAL_D TTime gTime2;
_LIT(KMsg1, "1st read timing: %d\n");
_LIT(KMsg2, "2nd read timing: %d\n");
_LIT(KMsg3, "3rd read timing: %d\n");

LOCAL_D RSemaphore gSync;

// Concurrent Threads
LOCAL_D RThread gThread1;
LOCAL_D RThread gThread2;
LOCAL_D RSemaphore client;
LOCAL_D const TInt KHeapSize = 0x4000;
LOCAL_D const TInt KMaxHeapSize = 0x100000;
LOCAL_D TBool gPagedRom = EFalse;

enum TTestState
	{
	EThreadWait,
	EThreadSignal,
	ENoThreads
	};

/** Formats the drive

	@param aDrive 	Drive to be formatted
	@param aFormatMode Mode for the format operation
*/
LOCAL_C void Formatting(TInt aDrive, TUint aFormatMode )
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf = _L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r = format.Open(TheFs,driveBuf,aFormatMode,count);
	test_KErrNone(r);
	while(count)
		{
		TInt r = format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}

/** Verifies the content of a buffer (all the letters are like the first one)

	@param aBuffer  Buffer to be verified

	@return KErrNone if all the letters are the same, KErrCorrupt otherwise
*/
LOCAL_C TInt VerifyBuffer(TDes8& aBuffer)
	{
	TChar c = aBuffer[0];

	for(TInt i = 1; i < aBuffer.Length(); i++)
		{
		if(i%32 != 0)
		{
			if(c != (TChar)(aBuffer[i] - 1))
				return KErrCorrupt;
		}
		else
		{
			if(aBuffer[i] != aBuffer[0])
				return KErrCorrupt;
		}
		c = aBuffer[i];
		}

	return KErrNone;
	}

/**  Fills a buffer with character aC, aC+1, aC+2, ..., aC+20, aC, etc

	@param aBuffer  Buffer to be filled, output
	@param aLength  Length to be filled
	@param aC		Character to be used to fill the buffer
*/
LOCAL_C void FillBuffer(TDes8& aBuffer, TInt aLength, TChar aC)
	{
	test (aBuffer.MaxLength() >= aLength);
	for(TInt i = 0; i < aLength; i++)
		{
		aBuffer.Append((i%32) + aC);
		}
	}

/**  Returns true if fat filesystem present on aDrive

	@param aFsSession 	Session on the File Server
	@param aDrive 		Drive to be looked at
*/
LOCAL_C TBool IsFSFAT(RFs &aFsSession,TInt aDrive)
	{
	TFileName f;
	TInt r = aFsSession.FileSystemName(f,aDrive);

	if (r != KErrNone)
		{
		test.Printf(_L("Unable to get file system name\n"));
		return EFalse;
		}

	return (f.CompareF(_L("Fat")) == 0);
	}

/** Generates a file name of the form FFFFF*<aPos>.TXT (aLong.3)

	@param aBuffer The filename will be returned here
	@param aLong   Defines the longitude of the file name
	@param aPos	   Defines the number that will be attached to the filename
*/
GLDEF_C void FileNameGen(TDes16& aBuffer, TInt aLong, TInt aPos)
{
	TInt padding;
	TInt i = 0;
	TBuf16<10> tempbuf;

	_LIT(KNumber,"%d");
	tempbuf.Format(KNumber,aPos);
	padding = aLong-tempbuf.Size()/2;
	aBuffer = _L("");
	while(i < padding)
	{
		aBuffer.Append('F');
		i++;
	}
	aBuffer.Append(tempbuf);

	_LIT(KExtension1, ".TXT");
	aBuffer.Append(KExtension1);
}

/**  Delete content of directory

	@param aDir	Target directory

	@return Error returned if any, otherwise KErrNone
*/
LOCAL_C TInt DeleteAll(TDes16& aDir)
{
	TBuf16<100> dir;
	CFileMan* fMan = CFileMan::NewL(TheFs);
	TInt r=0;

	dir = aDir;
	dir.Append(_L("F*.*"));
	r = fMan->Delete(dir);

	delete fMan;
	return r;
}

/**  Waits for all the TRequestStatus in status[] to complete

	@param status 	Array of TRequestStatus
	@param aLength  Length to be filled
	@param aC		Character to be used to fill the buffer
*/
LOCAL_C void WaitForAll(TRequestStatus* status, TInt aSize)
{
	TInt i = 0;

	RTest test(_L("T_RCACHE"));

	while(i < aSize)
		{
		User::WaitForRequest(status[i]);
		test(status[i] == KErrNone);
		i++;
		}

	test.Close();
}

/**  Reads the parameters from the comand line
	 and updates the appropriate variables
*/
LOCAL_C void parseCommandLine()
{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token = lex.NextToken();
	TInt r=0;

	if(token.Length() != 0)
		{
		gDriveToTest = token[0];
		gDriveToTest.UpperCase();
		}
	else
		{
		gDriveToTest = 'C';
		}

	r = TheFs.CharToDrive(gDriveToTest,gDrive);
	test_KErrNone(r);
	gSessionPath = _L("?:\\F32-TST\\");
	gSessionPath[0] = (TUint16) gDriveToTest;
	test.Printf(_L("\nCLP=%C\n"),(TInt)gDriveToTest);
}

/**  Writes a file synchronously in blocks of aBlockSize size

	@param fs			RFs object
	@param aFile		File name
	@param aSize		Size of the file in bytes
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aState		Determines if the operation is being done in the main process
						or in a thread
	@param aReadBack	Reads the file back if ETrue. This is used for Read-ahead testing to ensure
						what is written is also read back through the media driver's cache. If the
						file is bigger than the cache, then subsequent streaming reads starting from
						position zero will have to be re-read from the media.

	@return Returns KErrNone if everything ok, otherwise it panics
*/
LOCAL_C TInt WriteFile(RFs& fs, TDes16& aFile, TInt aSize, TInt aBlockSize, TDes8& aBuf, TTestState aState, TBool aReadBack = EFalse)
{
	RTest test(_L("T_RCACHE"));

	TInt r = 0;
	RFile fileWrite;
	TInt pos = 0;

	test(aBlockSize>0);
	test((aSize%aBlockSize) == 0); 	// Ensure the size of the file is a multiple of the block size

	if(aState == EThreadWait)
	{
		gSync.Wait();
	}

	// delete file first to ensure it's contents are not in the cache (file may be be on the closed file queue)
	r = fs.Delete(aFile);
	test_Value(r, r == KErrNone || r == KErrNotFound);

	r = fileWrite.Replace(fs,aFile,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
	test_KErrNone(r);

	TInt j = 0;
	while(j < aSize)
		{
			r = fileWrite.Write(pos, aBuf);
			test_KErrNone(r);

			if (aReadBack)
				{
				r = fileWrite.Read(pos, aBuf);
				test_KErrNone(r);
				}

			if((j>(aSize/2))&&(aState == EThreadSignal))
			{
				gSync.Signal();
				aState = ENoThreads;
			}
			j += aBlockSize;
			pos += aBlockSize;
		}
	fileWrite.Close();
	test.Close();

	return KErrNone;
}

/**  Read File in blocks of size aBlockSize

	@param fs			RFs object
	@param aFile		File name
	@param aBlockSize	Size of the blocks to be used in bytes

	@return Returns KErrNone if everything ok, otherwise it panics
*/
LOCAL_C TInt ReadFile(RFs& fs, TDes16& aFile, TInt aBlockSize)
{
	RTest test(_L("T_RCACHE"));

	TInt r = 0, size = 0;
	RFile fileRead;

	test(aBlockSize>0);				// Block size must be greater than 0

	r = fileRead.Open(fs,aFile,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
	test_KErrNone(r);

	r = fileRead.Size(size);
	test_KErrNone(r);

	TInt j = 0;
	while(j < size)
	{
		r = fileRead.Read(gBufReadPtr, aBlockSize);
		test_KErrNone(r);
		j += aBlockSize;
	}
	fileRead.Close();
	test.Close();

	return KErrNone;
}

/** Write a file asynchronously in blocks of aBlockSize size

	@param fs			RFs object
	@param aFileWrite	RFile object, needs to exist beyond the scope of this function
	@param aFile		File name
	@param aSize		Size of the file in bytes
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aStatus		TRequestStatus array for all the requests
*/
LOCAL_C void WriteFileAsync(RFs& fs, RFile& aFileWrite, TDes16& aFile, TInt aSize, TInt aBlockSize, TRequestStatus aStatus[])
{
	RTest test(_L("T_RCACHE"));

	TInt r = 0;

	test(aBlockSize>0);
	test((aSize%aBlockSize) == 0); 	// Ensure the size of the file is a multiple of the block size


	// delete file first to ensure it's contents are not in the cache (file may be be on the closed file queue)
	r = fs.Delete(aFile);
	test_Value(r, r == KErrNone || r == KErrNotFound);

	r = aFileWrite.Replace(fs,aFile,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
	test_KErrNone(r);

	TInt j = 0, i = 0;
	while(j < aSize)
	{
		aFileWrite.Write(gBufWritePtr,aStatus[i]);
		r = aStatus[i].Int();
		if (r != KErrNone && r != KRequestPending)
			{
			test.Printf(_L("Write %d returned %d\n"), i, r);
			test(0);
			}
		i++;

		j += aBlockSize;
	}
	test.Close();
}

/**  Read a file asynchronously in blocks of aBlockSize size

	@param fs			RFs object
	@param aFileRead	RFile object, needs to exist beyond the scope of this function
	@param aFile		File name
	@param aSize		Size of the file in bytes
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aStatus		TRequestStatus array for all the requests

	@return KErrNone
*/
LOCAL_C TInt ReadFileAsync(RFs& fs,RFile& aFileRead, TDes16& aFile, TInt aBlockSize,TRequestStatus aStatus[], TInt aFileSize)
	{
	RTest test(_L("T_RCACHE"));

	TInt r = 0;
	TInt size = 0;

	test(aBlockSize > 0);

	r = aFileRead.Open(fs,aFile,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
	test_KErrNone(r);

	r = aFileRead.Size(size);
	test_KErrNone(r);

	test(size == aFileSize);

	TInt j = 0, i = 0;
	while(j < size)
		{
		aFileRead.Read(gBufReadPtr,aStatus[i]);
		r = aStatus[i].Int();
		if (r != KErrNone && r != KRequestPending)
			{
			test.Printf(_L("Read %d returned %d\n"), i, r);
			test(0);
			}
		i++;

		j += aBlockSize;
		}

	test.Close();
	return KErrNone;
	}

/** Measure the time taken for this file to be written synchronously
*/
LOCAL_C TInt WriteTestFile(TDes16& aFile, TInt aSize, TBool aReadBack = EFalse)
{
	RTest test(_L("T_RCACHE"));

	TTime startTime;
	TTime endTime;
	TInt r = 0;

	startTime.HomeTime();

	r = WriteFile(TheFs,aFile, aSize, KBlockSize, gBufWritePtr, ENoThreads, aReadBack);
	test_KErrNone(r);

	endTime.HomeTime();

	gTimeTakenBigFile = I64LOW(endTime.MicroSecondsFrom(startTime).Int64());

	test.Close();
	return I64LOW(gTimeTakenBigFile.Int64()) / 1000;
}

/** Measure the time taken for this file to be read synchronously
*/
LOCAL_C TInt ReadTestFile(TDes16& aFile)
{
	TTime startTime;
	TTime endTime;

	startTime.HomeTime();
	ReadFile(TheFs,aFile, KBlockSize);
	endTime.HomeTime();

	gTimeTakenBigFile = I64LOW(endTime.MicroSecondsFrom(startTime).Int64());

	return I64LOW(gTimeTakenBigFile.Int64()) / 1000;
}

/** Read asynchronously the test file from the disc

*/
LOCAL_C TInt ReadAsyncTestFile(TDes16& aFile, TInt aSize)
{
	RTest test(_L("T_RCACHE"));

	TTime startTime;
	TTime endTime;
	TRequestStatus status[KWaitRequestsTableSize];
	RFs fs;
	RFile file;

	TInt r = fs.Connect();
	test_KErrNone(r);

	startTime.HomeTime();

	ReadFileAsync(fs, file, aFile, KBlockSize, status, aSize);
	WaitForAll(status, aSize/KBlockSize);

	endTime.HomeTime();

	gTimeTakenBigFile = I64LOW(endTime.MicroSecondsFrom(startTime).Int64());

	file.Close();
	fs.Close();
	test.Close();
	return I64LOW(gTimeTakenBigFile.Int64()) / 1000;
}

/** Write a file for the simple case

*/
LOCAL_C TInt WriteFileT(TAny* )
{
	RTest test(_L("T_RCACHE"));
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);

	r = fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	gTime1.HomeTime();

	r = WriteFile(fs,gCurrentFile,gCurrentFileSize, KBlockSize,gBufWritePtr, ENoThreads);
	test_KErrNone(r);

	gTime2.HomeTime();

	fs.Close();
	test.Close();

	gTimeTakenBigFile = I64LOW(gTime2.MicroSecondsFrom(gTime1).Int64());

	client.Signal();

	return ETrue;
}

/** Write a file for the concurrent case

*/
LOCAL_C TInt WriteFileT2(TAny* )
{
	RTest test(_L("T_RCACHE"));
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);
	RFile file;
	TRequestStatus status[KWaitRequestsTableSize];


	r = fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	WriteFileAsync(fs,file,gFirstFile,gSecondFileSize, KBlockSize, status);
	WaitForAll(status,gSecondFileSize/KBlockSize);

	TInt size = 0;
	file.Size(size);
	test( size == gSecondFileSize );

	file.Close();
	fs.Close();

	test.Close();

	client.Signal();

	return ETrue;
}

/**  Write a file for the simple case

*/
LOCAL_C TInt ReadFileT(TAny* )
{
	RTest test(_L("T_RCACHE"));
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);

	r = fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	gTime1.HomeTime();

	ReadFile(fs,gCurrentFile, KBlockSize);

	gTime2.HomeTime();

	fs.Close();
	test.Close();

	gTimeTakenBigFile = I64LOW(gTime2.MicroSecondsFrom(gTime1).Int64());

	client.Signal();

	return ETrue;
}

/**  Simple case, cache effect shown

*/
LOCAL_C void TestSimpleRead()
{
	TInt r = 0;
	TInt time = 0;
	TInt time2 = 0;
	TInt time3 = 0;
	TInt tcreate = 0;

	test.Start(_L(""));

	test.Next(_L("File fits in: read sync + read sync + read async\n"));

	tcreate = WriteTestFile(gSecondFile, gSecondFileSize);
	test.Printf(_L("Time to create the file: %d ms\n"),tcreate);

	time = ReadTestFile(gSecondFile);
	test.Printf(KMsg1,time);
	time2 = ReadTestFile(gSecondFile);
	test.Printf(KMsg2,time2);
	time3 = ReadAsyncTestFile(gSecondFile,gSecondFileSize);
	test.Printf(KMsg3,time3);
#if !defined(__WINS__)
	if (gPagedRom)
		test.Printf(_L("Skipping timing test on paged ROM\n"));
	else
		test((time2 <= time) && (time3 < time));
#endif

	r = DeleteAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrInUse);

	// Simple case filling/reading the cache from different threads
	test.Next(_L("File fits in: read sync (another thread) + read sync + read async\n"));
	gCurrentFile = gSecondFile;
	gCurrentFileSize = gSecondFileSize;

	TBuf<20> buf = _L("Write File");
	r = gThread1.Create(buf,WriteFileT,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread1.Resume();
	client.Wait();

	tcreate = I64LOW(gTimeTakenBigFile.Int64()) / 1000;

	test.Printf(_L("Time to create the file from a thread: %d ms\n"),tcreate);

	buf = _L("Read File");
	r = gThread2.Create(buf,ReadFileT,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread2.Resume();
	client.Wait();

	gThread1.Close();
	gThread2.Close();

	time = I64LOW(gTimeTakenBigFile.Int64()) / 1000;
	test.Printf(KMsg1,time);
	time2 = ReadTestFile(gSecondFile);
	test.Printf(KMsg2,time2);
	time3 = ReadAsyncTestFile(gSecondFile,gSecondFileSize);
	test.Printf(KMsg3,time3);
#if !defined(__WINS__)
	if (gPagedRom)
		test.Printf(_L("Skipping timing test on paged ROM\n"));
	else
		test((time2<=time) && (time3<time));
#endif

	r = DeleteAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrInUse);


	test.Next(_L("File doesn't fit in: read sync + read sync + read async\n"));

	tcreate = WriteTestFile(gFirstFile,gFirstFileSize);
	test.Printf(_L("Time to create the file: %d ms\n"),tcreate);

	time = ReadTestFile(gFirstFile);
	test.Printf(KMsg1,time);
	time2 = ReadTestFile(gFirstFile);
	test.Printf(KMsg2,time2);
	time3 = ReadAsyncTestFile(gFirstFile,gFirstFileSize);
	test.Printf(KMsg3,time3);

	#if !defined(__WINS__)
	// this isn't valid as the file doesn't fit in the cache, so there's no reason why
	// the second read should be any faster than the first
	// test((time2 <= time) && (time3 < time));
	#endif


	r = DeleteAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrInUse);


	test.Next(_L("File doesn't fit in: read sync (another thread) + read sync + read async\n"));
	gCurrentFile = gFirstFile;
	gCurrentFileSize = gFirstFileSize;

	buf = _L("Write Big File");
	r = gThread1.Create(buf,WriteFileT,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread1.Resume();
	client.Wait();

	tcreate = I64LOW(gTimeTakenBigFile.Int64()) / 1000;

	test.Printf(_L("Time to create the file from a thread: %d ms\n"),tcreate);

	buf = _L("Read Big File");
	r = gThread2.Create(buf,ReadFileT,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread2.Resume();
	client.Wait();

	gThread1.Close();
	gThread2.Close();

	time = I64LOW(gTimeTakenBigFile.Int64()) / 1000;
	test.Printf(KMsg1,time);
	time2 = ReadTestFile(gFirstFile);
	test.Printf(KMsg2,time2);
	time3 = ReadAsyncTestFile(gFirstFile,gFirstFileSize);
	test.Printf(KMsg3,time3);

	#if !defined(__WINS__)
	// this isn't valid as the file doesn't fit in the cache, so there's no reason why
	// the second read should be any faster than the first
	// test((time2 <= time) && (time3 < time));
	#endif

	r = DeleteAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrInUse);

	test.End();
}

/**  Thread to create file and read from it

*/
LOCAL_C TInt ReadAnotherEntry(TAny* )
{
	RTest test(_L("T_RCACHE"));
	RFs fs;
	TInt r = fs.Connect();
	RFile file;
	HBufC8* lBufSec = NULL;
	TPtr8 lBufReadPtr(NULL, 0);

	TRAPD(res2,lBufSec = HBufC8::NewL(KBlockSize+1));
	test(res2 == KErrNone && lBufSec != NULL);
	lBufReadPtr.Set(lBufSec->Des());

	test_KErrNone(r);
	r = fs.SetSessionPath(gSessionPath);


	// delete file first to ensure it's contents are not in the cache (file may be be on the closed file queue)
	r = fs.Delete(gFirstFile);
	test_Value(r, r == KErrNone || r == KErrNotFound);

	r = file.Create(fs,gFirstFile,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);

	if(r == KErrAlreadyExists)
	{
		r = file.Open(fs,gFirstFile,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
		test_KErrNone(r);
	}

	r = file.Write(gBufWritePtr);
	test_KErrNone(r);

	client.Signal();

	FOREVER
		{
			r = file.Read(gBufReadPtr, KBlockSize);
			test_KErrNone(r);
		}

}

/**  Test the cache behaviour in repeated call situations

*/
LOCAL_C void TestRepeatedRead()
{
	TInt r = 0;
	TInt time = 0;
	TInt i = 0;
	TInt tcreate = 0;


	test.Start(_L(""));
	test.Next(_L("File fits in: read sync / read async \n"));

	tcreate = WriteTestFile(gSecondFile, gSecondFileSize);
	test.Printf(_L("Time to create the file: %d ms\n"),tcreate);
	while(i < 20)
	{
		if(!(i % 2))
		{
			time = ReadTestFile(gSecondFile);
			test.Printf(_L("%d)  Sync Read: %d\n"), i+1 , time);
		}
		else
		{
			time = ReadAsyncTestFile(gSecondFile,gSecondFileSize);
			test.Printf(_L("%d) Async Read: %d\n"), i+1 , time);
		}
		i++;
	}

	r = DeleteAll(gSessionPath);
	test_KErrNone(r);

	test.Next(_L("File fits in: read sync / read async, with another thread using the drive \n"));

	TBuf<20> buf = _L("Noise Thread");
	r = gThread1.Create(buf,ReadAnotherEntry,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	tcreate = WriteTestFile(gSecondFile, gSecondFileSize);
	test.Printf(_L("Time to create the file: %d ms\n"),tcreate);

	gThread1.Resume();
	client.Wait();

	i = 0;
	while(i < 20)
	{
		if(!(i%2))
		{
			time = ReadTestFile(gSecondFile);
			test.Printf(_L("%d)  Sync Read: %d\n"), i+1 , time);
		}
		else
		{
			time = ReadAsyncTestFile(gSecondFile,gSecondFileSize);
			test.Printf(_L("%d) Async Read: %d\n"), i+1 , time);
		}
		i++;
	}

	gThread1.Kill(KErrNone);
	gThread1.Close();

	r = DeleteAll(gSessionPath);
	test_KErrNone(r);

	test.End();
}

/**  Concurrent operations testing

*/
LOCAL_C void TestConcurrent()
{
	TInt r = 0;
	TInt time = 0;
	TInt time2 = 0;
	TInt time3 = 0;

	test.Start(_L("Write two files concurrently\n"));

	gCurrentFile = gSecondFile;
	gCurrentFileSize = gSecondFileSize;

	TBuf<20> buf = _L("Write Two Files 1");
	r = gThread1.Create(buf,WriteFileT,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	TBuf<20> buf2 = _L("Write Two Files 2");
	r = gThread2.Create(buf2,WriteFileT2,KDefaultStackSize*2,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread1.Resume();
	gThread2.Resume();
	client.Wait();
	client.Wait();

	test.Next(_L("Read the two files repeatedly\n"));

	test.Printf(_L("File 1:\n"));
	time = ReadTestFile(gSecondFile);
	test.Printf(KMsg1,time);
	time2 = ReadTestFile(gSecondFile);
	test.Printf(KMsg2,time2);
	time3 = ReadAsyncTestFile(gSecondFile,gSecondFileSize);
	test.Printf(KMsg3,time3);

	test.Printf(_L("File 2:\n"));

	time = ReadTestFile(gFirstFile);
	test.Printf(KMsg1,time);
	time2 = ReadTestFile(gFirstFile);
	test.Printf(KMsg2,time2);
	time3 = ReadAsyncTestFile(gFirstFile,gSecondFileSize);
	test.Printf(KMsg3,time3);

	test.End();

	r = DeleteAll(gSessionPath);
	test_KErrNone(r);
}

/**  Create file from other thread, to be cached

*/
LOCAL_C TInt CreateFile(TAny* )
{
	RTest test(_L("T_RCACHE"));
	RFs fs;
	TInt r = fs.Connect();
	test_KErrNone(r);

	r = fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	r = WriteFile(fs, gSecondFile, gSecondFileSize, KBlockSize, gBufWritePtr, EThreadSignal);
	test_KErrNone(r);

	return KErrNone;
}

LOCAL_C TBool FindPattern(TUint8 *aBuf, TUint8 *aPattern, TInt aLong, TInt *aOffSet)
{
	TInt i = 0;
	TBool found = EFalse;

	while((i < (aLong-4)) && !found)
	{
		found = (
			(aBuf[i] == aPattern[0])&&
			(aBuf[i+1] == aPattern[1])&&
			(aBuf[i+2] == aPattern[2])&&
			(aBuf[i+3] == aPattern[3])
			);
	i++;
	}

	if(found)
		*aOffSet = --i;

	return found;
}

/**  Corrupts the second file with Raw access

*/
LOCAL_C void CorruptSecondFileRaw()
{
	RRawDisk rDisk;
	TUint8 gBuffer[4] =
	{
		65,66,67,68
	};
	TUint8 gBufferB[4] =
	{
		33,33,33,33
	};
	TPtr8 gBufferBPtr(&gBufferB[0], 4, 4);

	TUint8 gBuffer2[KBlockSize];
	TPtr8 gBuffer2Ptr(&gBuffer2[0], KBlockSize);

	TInt r = rDisk.Open(TheFs,gDrive);
	test_KErrNone(r);

	TInt64 pos = 0;
	TBool found = EFalse;
	TInt offset = 0;

	while(!found)
	{
		rDisk.Read(pos, gBuffer2Ptr);
		found = FindPattern(gBuffer2, gBuffer, KBlockSize, &offset);
		pos += (KBlockSize);
	}
	pos -= (KBlockSize+1);
	pos = pos+offset;

	r = rDisk.Write(pos+4, gBufferBPtr);
	test_KErrNone(r);

	rDisk.Close();
}

/**  Modifies the second file

*/
LOCAL_C TInt CorruptSecondFile()
{
	TInt r = 0;
	RFile fileWrite;
	HBufC8* dummy = NULL;
	TPtr8 dummyPtr(NULL, 0);

	TRAPD(res,dummy = HBufC8::NewL(4));
	test_Value(res, res== KErrNone && dummy != NULL);

	dummyPtr.Set(dummy->Des());
	FillBuffer(dummyPtr, 4, '1');

	r = fileWrite.Open(TheFs,gSecondFile,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
	if(r != 0)
		return r;
	TInt pos = 30;
	r = fileWrite.Seek(ESeekStart,pos);

	r = fileWrite.Write(dummyPtr);
	if(r != 0)
		return r;

	fileWrite.Close();

	delete dummy;

	return KErrNone;
}

//
// Read the file verifying content
//
LOCAL_C TInt ReadTestFileVerif(TDes16& aFile, TBool aRaw)
{
	TTime startTime;
	TTime endTime;
	TInt r = 0, size = 0;
	RFile fileRead;
	TInt corrupt = 0;
	TBool isFat=IsFSFAT(TheFs,gDrive);

	startTime.HomeTime();

	r = fileRead.Open(TheFs,aFile,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
	test_KErrNone(r);

	r = fileRead.Size(size);
	test_KErrNone(r);

	TInt j = 0;
	while(j < size)
		{
			r = fileRead.Read(gBufReadPtr, KBlockSize);
			if(aRaw)
			{
				if(isFat)
				{
					test_KErrNone(r);
				}
				else
				{
				if(r == KErrCorrupt)
					corrupt++;
				}
			}
			else
			{
				test_KErrNone(r);
			}
			j += KBlockSize;
			r = VerifyBuffer(gBufReadPtr);
			if(r == KErrCorrupt)
				corrupt++;
		}

	fileRead.Close();

	test(corrupt>0); // Ensure the cache returns the changed content

	endTime.HomeTime();

	gTimeTakenBigFile = I64LOW(endTime.MicroSecondsFrom(startTime).Int64());

	return I64LOW(gTimeTakenBigFile.Int64()) / 1000;
}


/** Negative testing

*/
LOCAL_C void TestNegative()
{
	TInt r = 0;
	TInt time, time2, time3;
	TInt tcreate = 0;

	test.Start(_L(""));
	// Kill a thread while writing, then read content
	TBuf<20> buf = _L("A thread to kill");
	gCurrentFile = gSecondFile;
	gCurrentFileSize = gSecondFileSize;

	r = gThread1.Create(buf,CreateFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread1.Resume();
	gSync.Wait();
	gThread1.Kill(KErrGeneral);
	gThread1.Close();
	test.Next(_L("Read after killing the write in the middle\n"));
	time = ReadTestFile(gSecondFile);
	test.Printf(KMsg1,time);
	time2 = ReadTestFile(gSecondFile);
	test.Printf(KMsg2,time2);

	// Read async the content
	TInt size = 0;
	TRequestStatus status[KWaitRequestsTableSize];
	TTime startTime;
	TTime endTime;
	RFile fileRead;

	startTime.HomeTime();

	r = fileRead.Open(TheFs,gSecondFile,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
	test_KErrNone(r);

	r = fileRead.Size(size);
	test_KErrNone(r);

	TInt j = 0, i = 0;
	while(j < size)
		{
			fileRead.Read(gBufReadPtr,status[i++]);
			test_KErrNone(r);
			j += KBlockSize;
		}

	j = i;
	i = 0;
	while(i < j)
	{
		User::WaitForRequest(status[i++]);
	}
	fileRead.Close();
	endTime.HomeTime();
	gTimeTakenBigFile = I64LOW(endTime.MicroSecondsFrom(startTime).Int64());
 	time3 = I64LOW(gTimeTakenBigFile.Int64()) / 1000;

	test.Printf(KMsg3,time3);

	// Modify file in some position
	test.Next(_L("Overwrite partially a file\n"));
	Formatting(gDrive, EFullFormat);
	r = TheFs.MkDirAll(gSessionPath);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		test_KErrNone(r);
		}

	tcreate = WriteTestFile(gSecondFile, gSecondFileSize);
	test.Printf(_L("Time to create the file: %d ms\n"),tcreate);

	time = ReadTestFile(gSecondFile);
	test.Printf(KMsg1,time);

	time = ReadTestFile(gSecondFile);
	test.Printf(KMsg2,time);

	CorruptSecondFile();

	time = ReadTestFileVerif(gSecondFile,EFalse);
	test.Printf(KMsg1,time);

	time = ReadTestFileVerif(gSecondFile,EFalse);
	test.Printf(KMsg2,time);

	// Modify the file in disk, raw access
	test.Next(_L("Overwrite file with raw access\n"));

	Formatting(gDrive,EFullFormat);

	r = TheFs.MkDirAll(gSessionPath);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		test_KErrNone(r);
		}


	tcreate = WriteTestFile(gSecondFile, gSecondFileSize);
	test.Printf(_L("Time to create the file: %d ms\n"),tcreate);

	time = ReadTestFile(gSecondFile);
	test.Printf(KMsg1,time);

	time = ReadTestFile(gSecondFile);
	test.Printf(KMsg2,time);

	CorruptSecondFileRaw();

	time = ReadTestFileVerif(gSecondFile,ETrue);
	test.Printf(KMsg1,time);

	time = ReadTestFileVerif(gSecondFile,ETrue);
	test.Printf(KMsg2,time);


	r = DeleteAll(gSessionPath);
	test_KErrNone(r);

	test.End();
}

/**  Creates the files to fill the read cache

	@param aFiles 	 Number of files needed to fill the cache
	@param aFileSize The file size
*/
LOCAL_C void CreateFiles(TInt aFiles, TInt aFileSize)
{
	TInt i = 0, r = 0;
	RFile file;
	TBuf16<50> directory;

	TBuf16<50> path;
	TBuf16<50> buffer(50);

	directory = gSessionPath;

	test.Printf(_L("Creating %d files for filling the cache (size %d)\n"), aFiles, aFileSize);

	// create a big buffer to speed things up
	HBufC8* bigBuf = NULL;
	const TInt KBigBifferSize = 32 * 1024;
	TRAPD(res,bigBuf = HBufC8::NewL(KBigBifferSize));
	test_Value(res, res== KErrNone && bigBuf != NULL);

	TPtr8 bigBufWritePtr(NULL, 0);
	bigBufWritePtr.Set(bigBuf->Des());
	FillBuffer(bigBufWritePtr, KBigBifferSize, 'A');


	i = 0;
	while(i < aFiles)
	{
		if (i % 10 == 0)
			test.Printf(_L("Creating file %d of %d...\r"), i, aFiles);
		FileNameGen(buffer, 8, i+3) ;
		path = directory;
		path.Append(buffer);

		// delete file first to ensure it's contents are not in the cache (file may be on the closed file queue)
		r = TheFs.Delete(path);
		test_Value(r, r == KErrNone || r == KErrNotFound);

		r = file.Create(TheFs,path,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
		if(r == KErrAlreadyExists)
			r = file.Open(TheFs,path,EFileShareAny|EFileWrite|EFileReadDirectIO|EFileWriteDirectIO);
		TInt j = 0;
		while(j < aFileSize)
			{
			bigBufWritePtr.SetLength(Min(KBigBifferSize, aFileSize - j));
			r = file.Write(bigBufWritePtr);
			test_KErrNone(r);
			j += bigBufWritePtr.Length();
			}

		file.Close();
		i++;
	}
	test.Printf(_L("\nFiles created\n"));
	delete bigBuf;
}

/**  Fills the read cache

	@param aFile	 Array of files needed to fill the cache
	@param aFiles 	 Number of files needed to fill the cache
	@param aFileSize The file size
*/
LOCAL_C void FillCache(RFile aFile[KFilesNeededToFillCache], TInt aFiles, TInt aFileSize)
{
	TInt i = 0, r = 0;
	TBuf16<50> directory;

	TBuf16<50> path;
	TBuf16<50> buffer(50);
	HBufC8* buf = NULL;
	TPtr8 bufPtr(NULL, 0);

	TRAPD(res,buf = HBufC8::NewL(2));
	test_Value(res, res== KErrNone && buf != NULL);
	bufPtr.Set(buf->Des());

	directory = gSessionPath;

	i = 0;
	while(i < aFiles)
	{
		FileNameGen(buffer, 8, i+3) ;
		path = directory;
		path.Append(buffer);
		r = aFile[i].Open(TheFs,path,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
		test_KErrNone(r);

		TInt j = 0;
		while(j < aFileSize)
			{
				r = aFile[i].Read(j,bufPtr);
				test_KErrNone(r);
				j += 4*KOneK;
			}

		i++;
	}

	delete buf;
	test.Printf(_L("Cache filled\n"));
}

/** Fills the default cache

*/
LOCAL_C void TestFillCache()
{
	TInt nFiles = KFilesNeededToFillCache;
	TInt fSize = KDefaultCacheSize;
	RFile file[KFilesNeededToFillCache];

	if(gMediaSize> ((fSize * nFiles)+gSecondFileSize+gFirstFileSize))
	{
		test.Start(_L("Creating files for filling the cache\n"));
		CreateFiles(nFiles,fSize);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		TFileCacheStats startPageCacheStats;
		TInt r = controlIo(TheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list at beginning=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list at beginning=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
#endif
		FillCache(file,nFiles,fSize);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		r = controlIo(TheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list at end=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list at end=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
#endif
		TestSimpleRead();

		TInt i = 0;
		while( i < KFilesNeededToFillCache )
			{
			file[i++].Close();
			}

		test.End();
	}
	else
		test.Printf(_L("Skipping the fill of the cache due to lack of space in the current drive\n"));
}

/**	Overflow-safe tick deltas

*/
static TInt64 TicksToMsec(TUint32 aInitTicks, TUint32 aFinalTicks, TInt aFastCounterFreq)
	{
	TUint32 timeDelta;
	if (aFinalTicks >= aInitTicks)
		timeDelta = aFinalTicks - aInitTicks;
	else
		timeDelta = aFinalTicks + (KMaxTUint32 - aInitTicks);		// must've wrapped

	return TInt64(timeDelta) * TInt64(1000000) / TInt64(aFastCounterFreq);
	}

/** Read three blocks and waits for the read ahead on the File Server to do its job

*/
LOCAL_C void TestReadAhead()
	{
	TInt r = 0,tcreate;
	RFile fileRead;
	HBufC8* dummy = NULL;
	TPtr8 dummyPtr(NULL, 0);

	TUint32 initTicks = 0;
	TUint32 finalTicks = 0;

	//--Find out if the drive is sync/async at this point and print information
    TPckgBuf<TBool> drvSyncBuf;
    r = TheFs.QueryVolumeInfoExt(gDrive, EIsDriveSync, drvSyncBuf);
    test_KErrNone(r);
	const TBool bDrvSync = drvSyncBuf();
    if(bDrvSync)
		test.Printf(_L("Drive D: is synchronous\n"));
	else
		test.Printf(_L("Drive D: is asynchronous\n"));

	// use a fast counter as this is more accurate than using TTime
	TInt fastCounterFreq;
	r = HAL::Get(HAL::EFastCounterFrequency, fastCounterFreq);
	test_KErrNone(r);
	test.Printf(_L("HAL::EFastCounterFrequency %d\n"), fastCounterFreq);

	// Bind this thread to CPU 0. This is so that timer deltas don't drift from
	// scheduling - else, it causes spurious failures.
	if (UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0) > 1)
		(void)UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny *)0, 0);

	const TInt KReadLen = 28 * KOneK;

	TRAPD(res,dummy = HBufC8::NewL(KReadLen));
	test_Value(res, res== KErrNone && dummy != NULL);

	dummyPtr.Set(dummy->Des());

	test.Start(_L("Creating test file..."));


	tcreate = WriteTestFile(gFirstFile, gFirstFileSize, ETrue);
	test.Printf(_L("Time to create the file: %d ms\n"),tcreate);

	r = fileRead.Open(TheFs,gFirstFile,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOn);
	test_KErrNone(r);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TFileCacheStats fileCacheStats;
	r = controlIo(TheFs,gDrive, KControlIoFileCacheStats, fileCacheStats);
	test_KErrNone(r);
	TInt totalBytesRead = fileCacheStats.iUncachedBytesRead;
	test.Printf(_L("totalBytesRead %d\n"), totalBytesRead);
	TInt bytesRead = 0;
	TInt filePos = 0;
#endif

	const TInt KReadCount = 6;
	#define	PAGE_ROUND_UP(x) ((x + 4095) & (-4096))
	TInt expectedBytesRead[KReadCount] = 
		{
		PAGE_ROUND_UP(KReadLen),	// read #0 from media
		PAGE_ROUND_UP(KReadLen),	// read #1 from media
		PAGE_ROUND_UP(KReadLen*2),	// read #2 from media, read-ahead #1 of length KReadLen
		PAGE_ROUND_UP(KReadLen*2),	// read #3 from cache, read-ahead #2 of length KReadLen * 2
		PAGE_ROUND_UP(KReadLen*4),	// read #4 from cache, read-ahead #3 of length KReadLen * 4
		0,							// read #5 from cache, no read-ahead
		};
	TTimeIntervalMicroSeconds readTimes[KReadCount];

	for (TInt n=0; n<KReadCount; n++)
		{

		initTicks = User::FastCounter();
		r = fileRead.Read(dummyPtr);
		finalTicks = User::FastCounter();
		test_KErrNone(r);

		readTimes[n] = TicksToMsec(initTicks, finalTicks, fastCounterFreq);
		test.Printf(_L("%d: read time %d \n"), n, I64LOW(readTimes[n].Int64()));

		TInt readAheadTime = I64LOW(readTimes[0].Int64()) * expectedBytesRead[n] / expectedBytesRead[0];
		// Wait for the read ahead to be done 
		if (n >= 2)
			{
			test.Printf(_L("Wait %u uS for read-ahead ...\n"), readAheadTime);
			User::After(readAheadTime);
			}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// check the number of bytes read from the media is as expected. i.e. including the read-ahead length
		// Keep waiting if it's not for up to 10 seconds
		const TInt KMaxWaitTime = 10000000;	// 10 secs
		TInt waitTime;
		for (waitTime=0; waitTime <KMaxWaitTime; waitTime+= readAheadTime)
			{
			r = controlIo(TheFs,gDrive, KControlIoFileCacheStats, fileCacheStats);
			test_KErrNone(r);
			bytesRead = fileCacheStats.iUncachedBytesRead - totalBytesRead;
			TInt bytesReadExpected = Min(gFirstFileSize - filePos, expectedBytesRead[n]);

			test.Printf(_L("bytesRead %d, bytesReadExpected %d\n"), bytesRead, bytesReadExpected);

			if (bytesRead == bytesReadExpected)
				break;
			User::After(readAheadTime);
			}
		test(waitTime < KMaxWaitTime);
		totalBytesRead+= bytesRead;
		filePos += bytesRead;
#endif

#if !defined(__WINS__)
		// Read #3 should be able to be satisfied entirely from the cache, so should be quicker. If it's not quicker,
		// display a warning rather than failing, because the read-ahead might be hogging the CPU, delaying the read from the cache.
		if (n >= 3)
			{
			if (readTimes[n].Int64() >= readTimes[0].Int64())
				test.Printf(_L("WARNING: Subsequent read not faster despite read-ahead !!!\n"));
			}
#endif
		}
	fileRead.Close();

	r = DeleteAll(gSessionPath);
	test_KErrNone(r);

	delete dummy;
	test.End();
	}

/** Main tests function
*/
GLDEF_C void CallTestsL()
	{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	test.Printf(_L("Disabling Lock Fail simulation ...\n"));
	// turn OFF lock failure mode
	TBool simulatelockFailureMode = EFalse;
	TInt r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_KErrNone(r);
#endif

	TBuf16<45> dir;

	RProcess().SetPriority(EPriorityBackground);

	// FileNames/File generation
	test.Start(_L("Preparing the environmnet\n"));
	FileNameGen(gFirstFile, 8, gNextFile++);
	FileNameGen(gSecondFile, 8, gNextFile++);
	dir = gSessionPath;
	dir.Append(gFirstFile);
	gFirstFile = dir;
	dir = gSessionPath;
	dir.Append(gSecondFile);
	gSecondFile = dir;


	TRAPD(res,gBuf = HBufC8::NewL(KBlockSize+1));
	test_Value(res, res== KErrNone && gBuf != NULL);

	gBufWritePtr.Set(gBuf->Des());
	FillBuffer(gBufWritePtr, KBlockSize, 'A');

	TRAPD(res2,gBufSec = HBufC8::NewL(KBlockSize+1));
	test(res2 == KErrNone && gBufSec != NULL);
	gBufReadPtr.Set(gBufSec->Des());

	test.Next(_L("Negative test\n"));
	TestNegative();

	test.Next(_L("Simple cases, use of the cache from same location/different"));
	TestSimpleRead();

	test.Next(_L("Repeated reads, same file\n"));
	TestRepeatedRead();

	test.Next(_L("Read ahead testing\n"));
	TestReadAhead();

	test.Next(_L("Concurrent read cases\n"));
	TestConcurrent();

	test.Next(_L("Fill the cache, boundary testing\n"));
	TestFillCache();

	test.End();
	delete gBuf;
	delete gBufSec;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// turn lock failure mode back ON (if enabled)
	simulatelockFailureMode = ETrue;
	r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_KErrNone(r);
#endif

	}

/** Initialises semaphores and call the tests
*/
LOCAL_C void DoTests()
	{
	TInt r = 0;

	r = client.CreateLocal(0);
 	test_KErrNone(r);

  	r = gSync.CreateLocal(0);
 	test_KErrNone(r);


	r = TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	r = TheFs.MkDirAll(gSessionPath);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		test_KErrNone(r);
		}
	TheFs.ResourceCountMarkStart();
	TRAP(r,CallTestsL());
	if (r == KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		test_KErrNone(r);
		}
	}

/** Determines the space that can be used for the files

*/
TBool CheckForDiskSize()
{
	TVolumeInfo volInfo;
	TInt r = TheFs.Volume(volInfo, gDrive);
	test_KErrNone(r);
	gMediaSize = volInfo.iFree;
	gSecondFileSize = KBlockSize*92;
	gFirstFileSize = KBlockSize*(256);
	while(((2*gFirstFileSize)+KOneMeg) > gMediaSize )
		{
			gFirstFileSize -= (2*KBlockSize);
		}

	TReal32 small = (TReal32)(gSecondFileSize/KOneK);
	TReal32 big = (TReal32)(gFirstFileSize/KOneK);

	test.Printf(_L("Test File: %.2f KB\n"), small );
	test.Printf(_L("Too big for the cache file: %.2f KB (%.2f MB)\n"), big, big / KOneK );

	if(gFirstFileSize < gSecondFileSize)
		return EFalse;
	else
		return ETrue;
}

/** Main function

	@return KErrNone if everything was ok, panics otherwise
*/
GLDEF_C TInt E32Main()
    {
	// Determine whether this is a paged ROM -
	// if it is we bypass some of the timimg tests as the default paging ROMs have a deliberately
	// small pool of pages (in order to stress the system) and reading things through the file cache
	// in this "artificial" environment can cause code to be evicted which can result in the read timings
	// going AWOL. In a more real-world environment, file caching should turn itself off if the amount of
	// memory falls below a threshold.
#if !defined(__WINS__)
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	gPagedRom = romHeader->iPageableRomStart ? (TBool)ETrue : (TBool)EFalse;
#endif

	RThread t;
	gMainThreadId = t.Id();

	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();

	__UHEAP_MARK;
	test.Start(_L("Starting tests... T_RCACHE"));
	parseCommandLine();

	TInt r = TheFs.Connect();
	test_KErrNone(r);

	TDriveInfo info;
	TVolumeInfo volInfo;
	r = TheFs.Drive(info,gDrive);
	test_KErrNone(r);

	if(info.iMediaAtt&KMediaAttVariableSize)
		{
		test.Printf(_L("Tests skipped in RAM drive\n"));
		goto out;
		}

	r = TheFs.Volume(volInfo, gDrive);
	if (r == KErrNotReady)
		{
		if (info.iType == EMediaNotPresent)
			test.Printf(_L("%c: Medium not present - cannot perform test.\n"), (TUint)gDriveToTest);
		else
			test.Printf(_L("%c: medium found (type %d) but drive not ready\nPrevious test may have hung; else, check hardware.\n"), (TUint)gDriveToTest, (TInt)info.iType);
		}
	else if (r == KErrCorrupt)
		{
		test.Printf(_L("%c: Media corruption; previous test may have aborted; else, check hardware\n"), (TUint)gDriveToTest);
		}
	test_KErrNone(r);

	if(!(volInfo.iFileCacheFlags & (EFileCacheReadEnabled | EFileCacheReadAheadEnabled)))
		{
		test.Printf(_L("Skipping tests, Read caching not enabled in this drive\n"));
		goto out;
		}
	
	if (((volInfo.iDrive.iMediaAtt & KMediaAttFormattable)))
		Formatting(gDrive,ESpecialFormat);

	if(CheckForDiskSize())
		{
		DoTests();
		}
	else
		{
		test.Printf(_L("Skipping tests due to lack of space to perform them in this drive\n"));
		}
out:
	test.End();

	TheFs.Close();
	test.Close();

	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }

