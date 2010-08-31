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
// f32test\server\t_fsched.cpp
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
#include <f32dbg.h>
#include "t_server.h"
#include <e32twin.h>
#include <e32cmn.h>
#include <hal.h>
#include "tf32testtimer.h"
#include "f32_test_utils.h"
using namespace F32_Test_Utils;


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FSCHED-0191
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ914
//! @SYMTestCaseDesc    This test case is exercising the Fair Scheduling functionality added to 
//!						the File Server. There are negative and positive tests. 
//! @SYMTestActions     0   setup the environment to execute the tests and benchmarks the time 
//!							taken to write the two files that are going to be used during the test
//!						1	TestReadingWhileWriting uses two threads (sync case) to write a big file/
//!							read a small file simultaneously and verifies that the timing is correct. 
//!							Same test in the async case. 
//!						2	TestWritingWhileWriting same test as before, but the two operations are 
//!							writes. 
//!						3 	TestTwoBigOnes does the same test as 1, but with two big files, ensuring
//!							that the first starting is the first finishing. 
//!						4 	TestReadingWhileWritingSameFile reads a file while it is being written. 
//!						5	TestClientDies starts a file write and kills it, then checks the integrity 
//!							of the volume. 
//!						6 	Finally it performs the benchmark of writing the two files again, to see 
//!							if it has been degraded during the execution
//!
//! @SYMTestExpectedResults finishes if the fair scheduling behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------


class RTestThreadSemaphore : public RSemaphore
	{
public:
	inline void Signal(TInt aError = KErrNone) {iError = aError; RSemaphore::Signal();};
public:
	TInt iError;
	};

// macro to be used in place of test(). Signals main thread if assertion fails
#define TEST(x)								\
if (!(x))									\
	{										\
	RThread t;								\
	if (t.Id() != gMainThreadId)			\
		client.Signal(KErrGeneral); 		\
	test(x);								\
	}				
#define TESTERROR(x)						\
	if (x != KErrNone)						\
	{										\
	RThread t;								\
	if (t.Id() != gMainThreadId)			\
		RDebug::Print(_L("error %d\n"), x);	\
	else									\
		test.Printf(_L("error %d\n"), x);	\
	TEST(x==KErrNone);								\
	}

#define CLIENTWAIT() {client.Wait(); test(client.iError == KErrNone);}

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


GLDEF_D RTest test(_L("T_FSCHED"));

GLDEF_D	RFs TheFs;
GLDEF_D TInt gDrive;
GLDEF_D TFileName gSessionPath;
GLDEF_D TChar gDriveToTest;	
TThreadId gMainThreadId;

HBufC8* gBuf = NULL;
TPtr8 gBufReadPtr(NULL, 0);	
HBufC8* gBufSec = NULL;
TPtr8 gBufWritePtr(NULL, 0);	

const TInt KOneK = 1024;
const TInt KOneMeg = KOneK * 1024;
const TInt KBigBlockSize = KOneMeg ; 
const TInt KBlockSize = KOneK * 129 ; 
const TInt KWaitRequestsTableSize = 70;

TInt gBigFileSize = 0; 
TInt gSmallFileSize = 0;
TInt64 gMediaSize = 0;
TInt gTotalTimeSync[2];
TInt gTotalTimeAsync[2];

TTimeIntervalMicroSeconds32 gTimeTakenBigFile(0);
TTimeIntervalMicroSeconds32 gTimeTakenBigBlock(0);
TBuf16<45> gSmallFile, gBigFile;
TInt gNextFile=0;

TF32TestTimer gTestTimer;

RSemaphore gSync;

// Concurrent Threads
RThread gBig;
RThread gSmall;
RTestThreadSemaphore client;	
const TInt KHeapSize = 0x4000;
const TInt KMaxHeapSize = 0x200000;
TRequestStatus gStatus[KWaitRequestsTableSize];

enum TTestState 
	{
	EThreadWait,
	EThreadSignal,
	ENoThreads	
	};




/** Generates a file name of the form FFFFF*<aPos>.TXT (aLong.3)

	@param aBuffer The filename will be returned here
	@param aLong   Defines the longitude of the file name 
	@param aPos	   Defines the number that will be attached to the filename
*/
void FileNameGen(TDes16& aBuffer, TInt aLong, TInt aPos) 
{
	TInt padding;
	TInt i=0;
	TBuf16<10> tempbuf;
	
	_LIT(KNumber,"%d");
	tempbuf.Format(KNumber,aPos);
	padding=aLong-tempbuf.Size()/2;
	aBuffer=_L("");
	while(i<padding)
	{
		aBuffer.Append('F');
		i++;
	}
	aBuffer.Append(tempbuf);
	
	_LIT(KExtension1, ".TXT");
	aBuffer.Append(KExtension1);
}


/**  Reads the parameters from the comand line 
	 and updates the appropriate variables
*/
void parseCommandLine() 
{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TInt r=0;

	if(token.Length()!=0)		
		{
		gDriveToTest = token[0];
		gDriveToTest.UpperCase();
		}
	else						
		{
		gDriveToTest='C';
		}	
		
	r = TheFs.CharToDrive(gDriveToTest,gDrive);
	TESTERROR(r);
	gSessionPath = _L("?:\\F32-TST\\");
	gSessionPath[0] = (TText)(gDrive+'A');
	test.Printf(_L("\nCLP=%C\n"), (TInt)gDriveToTest);
}

/** Verifies the content of a buffer (all the letters are like the first one)

	@param aBuffer  Buffer to be verified
	
	@return KErrNone if all the letters are the same, KErrCorrupt otherwise
*/
TInt VerifyBuffer(TDes8& aBuffer, TChar aExpectedChar)
	{
	TChar c = aExpectedChar;
	
	for(TInt i = 1; i<aBuffer.Length(); i++)
		{
		if(c != (TChar)(aBuffer[i])) 
			{
			RDebug::Print(_L("VerifyBuffer() failed at +%d, %02X != %02X"), i, (TInt) c, aBuffer[i] );
			return KErrCorrupt;
			}
		}
		
	return KErrNone;
	}

/**  Fills a buffer with character aC

	@param aBuffer  Buffer to be filled, output
	@param aLength  Length to be filled
	@param aC		Character to be used to fill the buffer
*/
void FillBuffer(TDes8& aBuffer, TInt aLength, TChar aC)
	{
	test (aBuffer.MaxLength() >= aLength);
	for(TInt i=0; i<aLength; i++)
		{
		aBuffer.Append(aC);
		}
	}

/**  Waits for all the TRequestStatus in status[] to complete

	@param status 	Array of TRequestStatus 
	@param aLength  Length to be filled
	@param aC		Character to be used to fill the buffer
*/
void WaitForAll(TRequestStatus* status, TInt aFileSize, TInt aBlockSize) 
{
	TInt i = 0;
	TInt size = 0;

	if((aFileSize % aBlockSize) == 0 ) 
		size = aFileSize/aBlockSize;	
	else 
		size = (aFileSize/aBlockSize) + 1;	

	while(i < size)
	{
		User::WaitForRequest(status[i++]);	
	}
}

/**  Writes a file synchronously in blocks of aBlockSize size

	@param fs			RFs object
	@param aFile		File name
	@param aSize		Size of the file in bytes
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aState		Determines if the operation is being done in the main process
						or in a thread
						
	@return Returns KErrNone if everything ok, otherwise it panics 
*/
TInt WriteFile(RFs& fs, TDes16& aFile, TInt aSize, TInt aBlockSize, TTestState aState) 
	{
	TInt r = 0;
	RFile fileWrite;

	TEST(aBlockSize>0);	
	
	r = fileWrite.Replace(fs,aFile,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	TESTERROR(r);

	if(aState==EThreadWait)
	{
		gSync.Wait();
		User::After(gTimeTakenBigBlock);
	}

	TInt j = 0;
	while(j < aSize)
		{
			if((j == 0) && (aState == EThreadSignal))
			{
				gSync.Signal();
			}			

			if((aSize - j) >= aBlockSize) 	// All the blocks but the last one
				r = fileWrite.Write(gBufWritePtr, aBlockSize); 
			else							// The last block
				r = fileWrite.Write(gBufWritePtr, (aSize - j)); 
				
			TESTERROR(r);
			j += aBlockSize;
		}					

	fileWrite.Close();
	
	return KErrNone;	
	}

/**  Read File in blocks of size aBlockSize

	@param fs			RFs object
	@param aFile		File name
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aState		Determines if the operation is being done in the main process
						or in a thread
						
	@return Returns KErrNone if everything ok, otherwise it panics 
*/
TInt ReadFile(RFs& fs, TDes16& aFile, TInt aBlockSize, TTestState aState) 
	{
	RTest test(_L("T_FSCHED"));
	TInt r = 0, size = 0;
	RFile fileRead;
	
	TEST(aBlockSize > 0);		

	r = fileRead.Open(fs, aFile, EFileShareAny|EFileRead|EFileReadDirectIO);
	TESTERROR(r);
	
	r = fileRead.Size(size);
	TESTERROR(r);
	
	if(aState == EThreadWait)
	{
		gSync.Wait();
		User::After(gTimeTakenBigBlock);
	}

	TInt j = 0;
	while(j < size) 
		{
			if((j == 0)&&(aState == EThreadSignal))
			{
				gSync.Signal();
			}			
			r = fileRead.Read(gBufReadPtr, aBlockSize);
			TESTERROR(r);
			r = VerifyBuffer(gBufReadPtr, 'B');	
			TESTERROR(r);
			
			j += aBlockSize;
			r = fileRead.Size(size);
			TESTERROR(r);
		}					

	fileRead.Close();

	return KErrNone;	
	}


/**  Write a file asynchronously in blocks of aBlockSize size

	@param fs			RFs object
	@param aFileWrite	RFile object, needs to exist beyond the scope of this function
	@param aFile		File name
	@param aSize		Size of the file in bytes
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aStatus		TRequestStatus array for all the requests
*/
void WriteFileAsync(RFs& fs, RFile& aFileWrite, TDes16& aFile, TInt aSize, TInt aBlockSize, TRequestStatus aStatus[]) 
	{
		
	TInt r = 0;
	
	TEST(aBlockSize > 0);				// Block size must be greater than 0

	r = aFileWrite.Replace(fs,aFile,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	TESTERROR(r);

	TInt j = 0;
	TInt i = 0;
	while(j < aSize)
		{
		if((aSize - j) >= aBlockSize) 	// All the blocks but the last one
			aFileWrite.Write(gBufWritePtr, aBlockSize, aStatus[i]); 
		else							// The last block
			aFileWrite.Write(gBufWritePtr, (aSize - j), aStatus[i]); 

		TInt status = aStatus[i].Int();
		if (status != KErrNone && status != KRequestPending)
			{
			test.Printf(_L("RFile::Write() returned %d\n"), aStatus[i].Int());
			}

		test (status == KErrNone || status == KRequestPending);
		i++;
		j += aBlockSize;	
		}					
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
TInt ReadFileAsync(RFs& fs, RFile& aFileRead, TDes16& aFile, TInt aBlockSize, TRequestStatus aStatus[]) 
	{
	TInt r = 0, size = 0;
	
	TEST(aBlockSize > 0);				// Block size must be greater than 0
	

	r = aFileRead.Open(fs, aFile, EFileShareAny|EFileRead|EFileReadDirectIO);
	TESTERROR(r);
	
	r = aFileRead.Size(size);
	TESTERROR(r);
	
	TInt j = 0, i = 0;
	while(j < size) 
		{
		aFileRead.Read(gBufReadPtr, aBlockSize, aStatus[i]);
		test (aStatus[i].Int() == KErrNone || aStatus[i].Int() == KRequestPending);
		i++;
		TESTERROR(r);
		j += aBlockSize;
		}					

	return KErrNone;	
	}


/**  
Write a pattern to 2 files simultaneously in blocks of aBlockSize size and verify that fair scheduling
does not interfere with the write order by reading both files back and verifying the contents.

aBlockSize should be slightly bigger that the fair scheduling size

@param aFs			RFs object
@param aSize		Size of the file in bytes
@param aBlockSize	Size of the blocks to be used in bytes, should be slightly bigger that the fair scheduling size
*/
void WriteOrderTest(RFs& aFs, TInt aSize, TInt aBlockSize) 
	{
	TFileName fileName1, fileName2;
	FileNameGen(fileName1, 8, 99);
	FileNameGen(fileName2, 8, 100);

	RFile file1, file2;

	TRequestStatus status1[KWaitRequestsTableSize];
	TRequestStatus status2[KWaitRequestsTableSize];

	TInt r = 0;
	
	TEST(aBlockSize > 0);				// Block size must be greater than 0

	r = file1.Replace(aFs, fileName1,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	TESTERROR(r);

	r = file2.Replace(aFs, fileName2,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	TESTERROR(r);


	HBufC8* buf1 = NULL;
	TRAP(r,buf1 = HBufC8::NewL(KBigBlockSize));
	TEST(r == KErrNone && buf1 != NULL);

	TInt maxBlockNum = KBigBlockSize / aBlockSize;
	TInt blockNum = 0;
	TInt blocksFree = maxBlockNum;

	TInt blockHead = 0;		// ha ha
	TInt blockTail = 0;


	TPtrC8* writeBuffer = new TPtrC8[maxBlockNum];
	for (TInt n=0; n<maxBlockNum; n++)
		{
		TUint8* addr = (TUint8*) buf1->Des().Ptr() + (n * aBlockSize);
		writeBuffer[n].Set(addr, aBlockSize);
		}

	TInt j = 0;
	TInt i = 0;
	while(j < aSize)
		{

		// fill the buffer with a char based on the block number
		TPtr8 bufWritePtr((TUint8*) writeBuffer[blockHead].Ptr(), 0, writeBuffer[blockHead].Length());
		TUint8* addr = (TUint8*) buf1->Des().Ptr() + (blockHead * aBlockSize);
		bufWritePtr.Set(addr, 0, aBlockSize);
		FillBuffer(bufWritePtr, aBlockSize, 'A'+blockNum);

//test.Printf(_L("j %d, addr %08X, aSize %d, blockNum %d, blockTail %d, blockHead %d, maxBlockNum %d, blocksFree %d\n"), 
//	j, writeBuffer[blockHead].Ptr(), aSize, blockNum, blockTail, blockHead, maxBlockNum, blocksFree);

		if((aSize - j) >= aBlockSize) 	// All the blocks but the last one
			file1.Write(writeBuffer[blockHead], aBlockSize, status1[blockHead]); 
		else							// The last block
			file1.Write(writeBuffer[blockHead], (aSize - j), status1[blockHead]); 

		if((aSize - j) >= aBlockSize) 	// All the blocks but the last one
			file2.Write(writeBuffer[blockHead], aBlockSize, status2[blockHead]); 
		else							// The last block
			file2.Write(writeBuffer[blockHead], (aSize - j), status2[blockHead]); 

		TInt status = status1[blockHead].Int();
		if (status != KErrNone && status != KRequestPending)
			test.Printf(_L("RFile::Write() returned %d\n"), status1[blockHead].Int());
		test (status == KErrNone || status == KRequestPending);

		status = status2[blockHead].Int();
		if (status != KErrNone && status != KRequestPending)
			test.Printf(_L("RFile::Write() returned %d\n"), status1[blockHead].Int());
		test (status == KErrNone || status == KRequestPending);

		i++;
		j += aBlockSize;	
		blockNum++;
		blockHead = (blockHead + 1) % maxBlockNum;
		blocksFree--;

		if (blocksFree == 0)
			{
//test.Printf(_L("Waiting for block %d\n"), blockTail);
			User::WaitForRequest(status1[blockTail]);
			User::WaitForRequest(status2[blockTail]);
			blockTail = (blockTail + 1) % maxBlockNum;
			blocksFree++;
			}

		}					
	
		while (blockTail != blockHead)
			{
//test.Printf(_L("Waiting for block %d\n"), blockTail);
			User::WaitForRequest(status1[blockTail]);
			User::WaitForRequest(status2[blockTail]);
			blockTail = (blockTail + 1) % maxBlockNum;
			blocksFree++;
			}

	file1.Close();	
	file2.Close();	

	// Verify file1
	r = file1.Open(aFs, fileName1, EFileShareAny|EFileRead|EFileReadDirectIO);
	TESTERROR(r);
	TInt size;
	r = file1.Size(size);
	TESTERROR(r);
	blockNum = 0;
	j = 0;
	while(j < size) 
		{
		TPtr8 readPtr((TUint8*) writeBuffer[0].Ptr(), aBlockSize, aBlockSize);
		r = file1.Read(readPtr);
		TESTERROR(r);
		r = VerifyBuffer(readPtr, 'A' + blockNum);	
		TESTERROR(r);
		j += aBlockSize;
		blockNum++;
		}	
	
	// Verify file2
	r = file2.Open(aFs, fileName2, EFileShareAny|EFileRead|EFileReadDirectIO);
	TESTERROR(r);
	r = file2.Size(size);
	TESTERROR(r);
	blockNum = 0;
	j = 0;
	while(j < size) 
		{
		TPtr8 readPtr((TUint8*) writeBuffer[0].Ptr(), aBlockSize, aBlockSize);
		r = file2.Read(readPtr);
		TESTERROR(r);
		r = VerifyBuffer(readPtr, 'A' + blockNum);	
		TESTERROR(r);
		j += aBlockSize;
		blockNum++;
		}	
	
	file2.Close();			
	file1.Close();			
	
	delete [] writeBuffer;
	delete buf1;

	r = aFs.Delete(fileName1);
	TEST(r == KErrNone);
	r = aFs.Delete(fileName2);
	TEST(r == KErrNone);
	}


/** Measure the time taken for a big block to be written synchronously
*/
void TimeTakenToWriteBigBlock() 
{
	RFile fileWrite;
	
	TInt r = fileWrite.Replace(TheFs,gBigFile,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	TESTERROR(r);

	// Calculate how long it takes to write a big block to be able to issue at the right time the concurrent writes
    TF32TestTimer timer;
	timer.Start();
	
	r = fileWrite.Write(gBufWritePtr, KBigBlockSize); 

    timer.Stop();

	fileWrite.Close();

	TESTERROR(r);
	
	gTimeTakenBigBlock = timer.TimeInMicroSeconds()/3;
        
	test.Printf(_L("\nTime spent to write the big block in isolation: %d ms\n"), TF32TestTimer::TimeInMilliSeconds(gTimeTakenBigBlock));
}
	

/** Measure the time taken for this file to be written synchronously
*/
void TimeTakenToWriteBigFile(TInt aPos) 
{
	test((aPos >= 0) && (aPos <= 1));
    TF32TestTimer timer;
    timer.Start();

	WriteFile(TheFs,gBigFile, gBigFileSize, KBigBlockSize, ENoThreads);
	
	timer.Stop();

	gTimeTakenBigFile = timer.Time32();
		
	test.Printf(_L("\nTime spent to write the big file in isolation: %d ms\n"), TF32TestTimer::TimeInMilliSeconds(gTimeTakenBigFile));
	
	gTotalTimeSync[aPos] = Max(TF32TestTimer::TimeInMilliSeconds(gTimeTakenBigFile), gTotalTimeSync[aPos]) ;
}

/** Measure the time taken for this file to be written asynchronously
*/
void TimeTakenToWriteBigFileAsync(TInt aPos) 
{
	TRequestStatus status[KWaitRequestsTableSize];
	RFile bigFile;
	
	test((aPos >= 0) && (aPos <= 1));

    TF32TestTimer2 timer;
	timer.Start();
	WriteFileAsync(TheFs, bigFile, gBigFile, gBigFileSize,KBigBlockSize,status); 
	timer.Stop();
	
	WaitForAll(status, gBigFileSize, KBigBlockSize);	
	timer.Stop2();
	
	gTimeTakenBigFile = timer.Time32();        
	bigFile.Close();
	test.Printf(_L("\nTime to queue the blocks in isolation asynchronously: %d ms, "), timer.TimeInMilliSeconds());
	gTimeTakenBigFile = timer.Time2();
	test.Printf(_L("to actually write it: %d ms\n"), TF32TestTimer::TimeInMilliSeconds(gTimeTakenBigFile));
	gTotalTimeAsync[aPos] = Max(TF32TestTimer::TimeInMilliSeconds(gTimeTakenBigFile), gTotalTimeAsync[aPos]) ; 
}

/**  Delete content of directory

	@param aDir	Target directory
	
	@return Error returned if any, otherwise KErrNone
*/
TInt DeleteAll(TDes16& aDir) 
	{
		TBuf16<100> dir;
		CFileMan* fMan=CFileMan::NewL(TheFs);
		TInt r=0;
		
		dir = aDir;
		dir.Append(_L("F*.*"));
		r = fMan->Delete(dir);	

		delete fMan;
		return r;
	}

/**  Read a small file sync, in a different thread

	@return ETrue
*/
TInt ReadSmallFile(TAny* )
	{
	RTest test(_L("T_FSCHED"));
	RFs fs;
	TInt r=fs.Connect();
	TESTERROR(r);

	r = fs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	
	ReadFile(fs,gSmallFile,KBlockSize, EThreadWait);
	gTestTimer.Start();
	
	client.Signal();
	
	fs.Close();
	test.Close();
	
	return ETrue;
	}

/**  Read the big file sync, in a different thread

	@return ETrue
*/
TInt ReadBigFile(TAny* )
	{
	RTest test(_L("T_FSCHED"));
	RFs fs;
	TInt r=fs.Connect();
	TESTERROR(r);

	r=fs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	
	ReadFile(fs,gBigFile,KBlockSize, EThreadWait);
	gTestTimer.Stop();
	
	client.Signal();
		
	fs.Close();
	test.Close();
	
	return ETrue;
	}

/**  Write a small file sync, in a different thread

	@return ETrue
*/
TInt WriteSmallFile(TAny* )
	{
	RTest test(_L("T_FSCHED"));
	RFs fs;
	TInt r=fs.Connect();
	TESTERROR(r);

	r=fs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	
	WriteFile(fs,gSmallFile,gSmallFileSize, KBlockSize, EThreadWait);
	gTestTimer.Start();
	
	client.Signal();
	
	fs.Close();
	test.Close();
	
	return ETrue;
	}

/**  Write a big file sync, in a different thread

	@return ETrue
*/
TInt WriteBigFile(TAny* )
	{
	RTest test(_L("T_FSCHED"));
	RFs fs;
	TInt r=fs.Connect();
	TESTERROR(r);

	r=fs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	
	WriteFile(fs, gBigFile, gBigFileSize, KBigBlockSize, EThreadSignal); 
	gTestTimer.Stop();
	
	client.Signal();
	
	fs.Close();
	test.Close();
	
	return ETrue;
	}

/**   Write big file after a signalling thread has been scheduled, in a different thread

	@return ETrue
*/
TInt WriteBigFile2(TAny* )
	{
	RTest test(_L("T_FSCHED"));
	RFs fs;
	TInt r=fs.Connect();
	TESTERROR(r);

	r = fs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	
	WriteFile(fs, gSmallFile, gBigFileSize, KBigBlockSize, EThreadWait); 
	gTestTimer.Stop();
	
	client.Signal();
	
	fs.Close();
	test.Close();
	
	return ETrue;
	}

/**  Write a big file async, in a different thread

	@return ETrue
*/
TInt WriteBigFileAsync(TAny* )
	{
	RTest test(_L("T_FSCHED"));
	RFs fs;
	TInt r = fs.Connect();
	TESTERROR(r);

	r=fs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	

	RFile bigFile;
	
	WriteFileAsync(fs, bigFile, gSmallFile, gBigFileSize, KBlockSize,gStatus); 
	gSync.Signal();
	WaitForAll(gStatus, gBigFileSize, KBlockSize);
	
	fs.Close();
	test.Close();
	
	return ETrue;
	}
	
/**  Delete a big file, in a different thread

	@return ETrue
*/
TInt DeleteBigFile(TAny* )
	{
	RTest test(_L("T_FSCHED"));
	RFs fs;
	TInt r = fs.Connect();
	TESTERROR(r);

	r = fs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	
	gSync.Wait(); 
	
	r = fs.Delete(gBigFile);
#if defined(__WINS__)
	TEST(r == KErrInUse || r == KErrNone);
#else
	test_Equal(KErrInUse, r);
#endif
		
	client.Signal();
	
	fs.Close();
	test.Close();
	
	return ETrue;
	}

/**  Reads a small file while writing a big one

*/
void TestReadingWhileWriting() 
{	
	// Write the small file and take the appropriate measures
	WriteFile(TheFs,gSmallFile,KBlockSize, KBlockSize, ENoThreads);

	// Sync test
	TBuf<20> buf=_L("Big Write");
	TInt r = gBig.Create(buf, WriteBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);

	buf = _L("Small Read");
	r = gSmall.Create(buf, ReadSmallFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);
	
	gBig.Resume();
	gSmall.Resume();

	CLIENTWAIT();
	CLIENTWAIT();
	
	gBig.Close();
	gSmall.Close();
	
	TTimeIntervalMicroSeconds timeTaken = gTestTimer.Time();
	test.Printf(_L("\nSync read done %d ms before the write ended\n"), TF32TestTimer::TimeInMilliSeconds(timeTaken));
	#if !defined(__WINS__)
		// If this condition fails, it means that writing the sync file while 
        // fairscheduling a small sync read takes too long. Reading small file
        // should complete first.
		test(timeTaken > 0);
	#endif 
	
	// Async test 
	TRequestStatus status[KWaitRequestsTableSize];
	TRequestStatus status2[2];
	RFile bigFile, smallFile;	

	WriteFileAsync(TheFs, bigFile, gBigFile, gBigFileSize,  KBigBlockSize ,status); 
	ReadFileAsync(TheFs, smallFile, gSmallFile, KBlockSize, status2 );

	WaitForAll(status2,KBlockSize , KBlockSize );

    TF32TestTimer timer;
	timer.Start();

	WaitForAll(status,gBigFileSize, KBigBlockSize);

	timer.Stop();
	bigFile.Close();
	smallFile.Close();
	
	timeTaken = timer.Time();
	
	test.Printf(_L("\nAsync read done %d ms before the write ended\n"), timer.TimeInMilliSeconds());

	#if !defined(__WINS__)
	if (!Is_HVFS(TheFs, gDrive))
		{
		// If this condition fails, it means that writing the async file while fairscheduling a small async read takes too long
		test.Printf(_L("gTotalTimeAsync[0] = %d , gTotalTimeAsync[1] = %d\n"),gTotalTimeAsync[0],gTotalTimeAsync[1] );
		test(timeTaken > 0);
		}
	#endif
}

/** Writes a small file while writing a big one

*/
void TestWritingWhileWriting() 
{
	TInt r = 0;

	// Sync test
	TBuf<20> buf = _L("Big Write II");
	r = gBig.Create(buf, WriteBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);

	buf = _L("Small Write II");
	r = gSmall.Create(buf, WriteSmallFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r==KErrNone);
	
	gBig.Resume();
	gSmall.Resume();

	CLIENTWAIT();
	CLIENTWAIT();
	
	gBig.Close();
	gSmall.Close();
	
	TTimeIntervalMicroSeconds timeTaken = gTestTimer.Time();
	test.Printf(_L("\nSync write done %d ms before the big write ended\n"), gTestTimer.TimeInMilliSeconds());
	#if !defined(__WINS__)
		// If this condition fails, it means that writing the sync file while 
        // fairscheduling a small sync write takes too long		
		test(timeTaken > 0);
	#endif 

	// Async test 
	TRequestStatus status[KWaitRequestsTableSize];
	TRequestStatus status2[1];
	RFile bigFile, smallFile;

	WriteFileAsync(TheFs, bigFile, gBigFile, gBigFileSize, KBigBlockSize, status); 
	WriteFileAsync(TheFs,smallFile, gSmallFile,gSmallFileSize,KBlockSize,status2);
	WaitForAll(status2,gSmallFileSize, KBlockSize);
    TF32TestTimer timer;
	timer.Start();
	WaitForAll(status, gBigFileSize, KBigBlockSize);
	timer.Stop();
	
	timeTaken = timer.Time();
	test.Printf(_L("\nAsync write done %d ms before the big write ended\n"),timer.TimeInMilliSeconds());
	#if !defined(__WINS__)
	if (!Is_HVFS(TheFs, gDrive))
		{
		// If this condition fails, it means that writing the async file while fairscheduling a small async write takes too long
		test.Printf(_L("gTotalTimeAsync[0] = %d , gTotalTimeAsync[1] = %d\n"),gTotalTimeAsync[0],gTotalTimeAsync[1] );
		test(timeTaken > 0);
		}
	#endif
	bigFile.Close();
	smallFile.Close();	
}

/** Writes two big files, ensuring the one that started to be written later ends the last one

*/
void TestTwoBigOnes()
{
	TInt r = 0;

	// Sync test
	TBuf<20> buf = _L("Big Write IIII");
	r = gBig.Create(buf, WriteBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);

	buf = _L("Big Write The Second");
	r = gSmall.Create(buf, WriteBigFile2, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);

	gBig.Resume();
	gSmall.Resume();

	CLIENTWAIT();
	CLIENTWAIT();
	
	gBig.Close();
	gSmall.Close();
	
	TTimeIntervalMicroSeconds timeTaken = gTestTimer.Time();
	test.Printf(_L("\nSync first write ended %d ms before the second write ended (same file size)\n"),TF32TestTimer::TimeInMilliSeconds(timeTaken));

	// Async test 
	TRequestStatus status[KWaitRequestsTableSize];
	TRequestStatus status2[KWaitRequestsTableSize];
	RFile bigFile, bigFile2;

	WriteFileAsync(TheFs, bigFile, gBigFile, gBigFileSize, KBigBlockSize, status); 
	WriteFileAsync(TheFs, bigFile2, gSmallFile, gBigFileSize, KBigBlockSize, status2);
	WaitForAll(status, gBigFileSize, KBigBlockSize);
    TF32TestTimer timer;
	timer.Start();
	WaitForAll(status2, gBigFileSize, KBigBlockSize);
	timer.Stop();
	
	timeTaken = timer.Time();
	test.Printf(_L("\nAsync first write ended %d ms before the second write ended (same file size)\n"),TF32TestTimer::TimeInMilliSeconds(timeTaken));
	bigFile.Close();
	bigFile2.Close();	
}

/**  Reads the file that is being written

*/
void TestReadingWhileWritingSameFile() 
{
	TInt r = 0;
	TF32TestTimer timer;
	
	timer.Start();
	
	// Sync test
	TBuf<20> buf = _L("Big Write IV");
	r = gBig.Create(buf, WriteBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);

	buf = _L("Read IV");
	r = gSmall.Create(buf, ReadBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);
	
	gBig.Resume();
	gSmall.Resume();
	
	CLIENTWAIT();
	CLIENTWAIT();
	
	CLOSE_AND_WAIT(gBig);
	CLOSE_AND_WAIT(gSmall);
	
	TTimeIntervalMicroSeconds timeTaken = gTestTimer.Time();
	test.Printf(_L("\nSync write finished %d ms before the read ended\n"),TF32TestTimer::TimeInMilliSeconds(timeTaken));
	
}

/** Client dying uncleanly before the operation is finished

*/
void TestClientDies()
{
	TInt r = 0;


	TBuf<20> drive = _L("?:\\");
	TVolumeInfo volInfo;
	drive[0]=(TText)(gDrive+'A');
	
	r = TheFs.CheckDisk(drive);
	TEST(r == KErrNone || r == KErrNotSupported);

	// Sync test
	TBuf<20> buf = _L("Big Write V");
	r = gBig.Create(buf, WriteBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);
	
	TRequestStatus status;
	gBig.Logon(status);
	gBig.Resume();
	gSync.Wait();

	// Kill the writing thread and wait for it to die.
	
    if(status.Int() == KRequestPending)
		{// the people who wrote this test did not consider the case when the file write finishes before they try to kill the thread.
		gBig.Kill(KErrGeneral);
		User::WaitForRequest(status);
		TEST(gBig.ExitReason() == KErrGeneral);
		TEST(gBig.ExitType() == EExitKill);
		}

    
    // Make sure the thread is destroyed and the handles it owned and IPCs
    // it executed are closed/cancelled.
    CLOSE_AND_WAIT(gBig);


	r = TheFs.Volume(volInfo, gDrive);
	TESTERROR(r);

	r = TheFs.CheckDisk(drive);
	TEST(r == KErrNone || r == KErrNotSupported);

	r = TheFs.ScanDrive(drive);
	TEST(r == KErrNone || r == KErrNotSupported);

	test.Printf(_L("Sync operation stopped\n"));
	
	// Async test 
	buf = _L("Big Write VI");
	r = gSmall.Create(buf, WriteBigFileAsync, KDefaultStackSize * 2, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);
	
	gSmall.Logon(status);
	gSmall.Resume();
	gSync.Wait();
	
    
    if(status.Int() == KRequestPending)
		{
		// Kill the writing thread and wait for it to die.
		gSmall.Kill(KErrGeneral);
		User::WaitForRequest(status);
		TEST(gSmall.ExitReason() == KErrGeneral);
		TEST(gSmall.ExitType() == EExitKill);
		}


    // Make sure the thread is destroyed and the handles it owned and IPCs
    // it executed are closed/cancelled.
    CLOSE_AND_WAIT(gSmall);

	r = TheFs.CheckDisk(drive);
	TEST(r == KErrNone || r == KErrNotSupported);
	
	r=TheFs.ScanDrive(drive);
	TEST(r == KErrNone || r == KErrNotSupported);
	
	test.Printf(_L("Async operation stopped\n"));
}

/** Reads a small file while writing a big one

*/
void TestDeletionWhileWriting()
{
	TInt r = 0;
	
	// Sync test
	TBuf<20> buf = _L("Big Write V");
	r = gBig.Create(buf, WriteBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);

	buf = _L("Deletion");
	r = gSmall.Create(buf, DeleteBigFile, KDefaultStackSize, KHeapSize, KMaxHeapSize, NULL);
	TEST(r == KErrNone);

	gSmall.Resume();
	gBig.Resume();

	CLIENTWAIT();
	CLIENTWAIT();
	
	gBig.Close();
	gSmall.Close();
	
	test.Printf(_L("The file was properly blocked when writing sync, not deleted\n"));
	
	// Async test 
	TRequestStatus status[KWaitRequestsTableSize];
	RFile bigFile;
	RFs fs; 

	r = fs.Connect();
	TESTERROR(r);
	r = fs.SetSessionPath(gSessionPath);
	TESTERROR(r);

	WriteFileAsync(TheFs, bigFile, gBigFile, gBigFileSize, KBigBlockSize, status); 	

	r = fs.Delete(gBigFile);
	TEST(r == KErrInUse);
	
	WaitForAll(status, gBigFileSize, KBigBlockSize);
	
	bigFile.Close();
	fs.Close();
	
	test.Printf(_L("The file was properly blocked when writing async, not deleted\n"));

}


void TestWriteOrder()
	{
	RFs fs; 

	TInt r = fs.Connect();
	TESTERROR(r);
	r = fs.SetSessionPath(gSessionPath);
	TESTERROR(r);

	WriteOrderTest(TheFs, gBigFileSize, KBlockSize);

	fs.Close();
	}


/** Main tests function
*/ 
void CallTestsL()
	{
	TBuf16<45> dir;
	TInt r = 0;
	
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	test.Printf(_L("Disabling Lock Fail simulation ...\n"));
	// turn OFF lock failure mode (if cache is enabled)
	
	TBool simulatelockFailureMode = EFalse;
	r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_Value(r, r == KErrNone  ||  r == KErrNotSupported);
#endif

	// FileNames/File generation
	test.Start(_L("Preparing the environment\n"));
	
	FileNameGen(gSmallFile, 8, gNextFile++);
	FileNameGen(gBigFile, 8, gNextFile++);
	
	dir = gSessionPath;
	dir.Append(gSmallFile);
	
	gSmallFile = dir;
	dir = gSessionPath;
	dir.Append(gBigFile);
	gBigFile = dir;

	TRAPD(res,gBuf = HBufC8::NewL(KBigBlockSize));
	TEST(res == KErrNone && gBuf != NULL);
		
	gBufWritePtr.Set(gBuf->Des());
	FillBuffer(gBufWritePtr, KBigBlockSize, 'B');
	
	TRAPD(res2, gBufSec = HBufC8::NewL(KBlockSize));
	TEST(res2 == KErrNone && gBufSec != NULL);
	gBufReadPtr.Set(gBufSec->Des());
	
	test.Next(_L("Benchmarking\n"));
	TimeTakenToWriteBigFile(0);  
	TimeTakenToWriteBigFileAsync(0);
	
	test.Printf(_L("second try, second timings account for the last comparison\n")); 
	TimeTakenToWriteBigFile(0);  
	TimeTakenToWriteBigFileAsync(0);
	
	TimeTakenToWriteBigBlock();
	
	test.Next(_L("Big file sync written, small file read from the media at the same time\n"));
	TestReadingWhileWriting();
	
	test.Next(_L("Big file written, small file written at the same time\n"));
	TestWritingWhileWriting();
	
  	test.Next(_L("Big file written async, deletion in the meantime\n"));
	TestDeletionWhileWriting();
	
	test.Next(_L("Two big files written at the same time\n"));
	TestTwoBigOnes();	
	
	test.Next(_L("Big file being written, start reading\n"));
	TestReadingWhileWritingSameFile();

	test.Next(_L("Client dying unexpectedly\n"));
	TestClientDies();

  	test.Next(_L("Ensure write order is preserved\n"));
	TestWriteOrder();
	
	// Format the drive to make sure no blocks are left to be erased in LFFS
	if (!Is_Win32(TheFs, gDrive))
		Format(gDrive);	
	
	r = TheFs.MkDirAll(gSessionPath);
	
	TimeTakenToWriteBigFile(1);  
	TimeTakenToWriteBigFileAsync(1);

	// Make sure that the difference between the first time and the last time the files are written doesn't
	// differ more than 3%
	test.Printf(_L("Abs(gTotalTimeSync[0]-gTotalTimeSync[1]) :%d\n"), Abs(gTotalTimeSync[0]-gTotalTimeSync[1]));
	test.Printf(_L("Abs(gTotalTimeAsync[0]-gTotalTimeAsync[1]) :%d\n"), Abs(gTotalTimeAsync[0]-gTotalTimeAsync[1]));
	test.Printf(_L("gTotalTimeSync[0] :%d\n"), gTotalTimeSync[0]);
	test.Printf(_L("gTotalTimeAsync[0] :%d\n"), gTotalTimeAsync[0]);
	
	#if !defined(__WINS__)
		test((Abs(gTotalTimeSync[0]-gTotalTimeSync[1])/gTotalTimeSync[0]) < 0.03);
		test((Abs(gTotalTimeAsync[0]-gTotalTimeAsync[1])/gTotalTimeAsync[0]) < 0.03);	
	#endif
	
	r = DeleteAll(gSessionPath);
	TESTERROR(r);
	
	delete gBuf;
	delete gBufSec;
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// turn lock failure mode back ON (if cache is enabled)
	simulatelockFailureMode = ETrue;
	r = controlIo(TheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
    test_Value(r, r == KErrNone  ||  r == KErrNotSupported);
#endif

	test.End();
	}

/** Initialises semaphores and call the tests
*/ 
void DoTests()
	{
 	TInt r = 0;
 	
 	r = client.CreateLocal(0);
 	TESTERROR(r);
 
  	r = gSync.CreateLocal(0);
 	TESTERROR(r);
 
	r = TheFs.SetSessionPath(gSessionPath);
	TESTERROR(r);
	
	r = TheFs.MkDirAll(gSessionPath);
	if (r != KErrNone && r != KErrAlreadyExists)
		{
		TESTERROR(r);
		}
	TheFs.ResourceCountMarkStart();
	TRAP(r,CallTestsL());
	if (r == KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		TESTERROR(r);
		}
	}

/** Determines the space that can be used for the files

*/
TBool CheckForDiskSize()
{
	TVolumeInfo volInfo;
	TInt r = TheFs.Volume(volInfo, gDrive);
	TESTERROR(r);
	
	gMediaSize = volInfo.iFree;
	gSmallFileSize = KBlockSize;
	gBigFileSize = KBlockSize*20;
	
	while(((2*gBigFileSize)+KOneMeg) > gMediaSize ) 
		{
		gBigFileSize -= (2*KBlockSize);
		}

	TReal32 small = (TReal32)(gSmallFileSize/KOneK);
	TReal32 big = (TReal32)(gBigFileSize/KOneK);
	
	
	test.Printf(_L("Small File Size: %.2f KB\n"), small); 
	test.Printf(_L("Big File Size: %.2f KB (%.2f MB)\n"), big, big / KOneK); 
	
	if(gBigFileSize< (3*gSmallFileSize)) 
		return EFalse;
	else 
		return ETrue;
}

/** Formats the drive 

	@param aDrive 	Drive to be formatted
*/
void Format(TInt aDrive)
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf = _L("?:\\");
	driveBuf[0] = (TText)(aDrive+'A');
	RFormat format;
	TInt count, prevcount = 0;
	TInt r = format.Open(TheFs, driveBuf, EQuickFormat, count);
	TESTERROR(r);
	
	while(count)
		{
		TInt r = format.Next(count);
        if(count != prevcount)
	        {
			test.Printf(_L("."));
			prevcount = count;
			}
		TESTERROR(r);
		}

	format.Close();
	}

/** Main function

	@return KErrNone if everything was ok, panics otherwise
*/
TInt E32Main()
    {
	RThread t;
	gMainThreadId = t.Id();
	
	CTrapCleanup* cleanup;
	cleanup = CTrapCleanup::New();

	__UHEAP_MARK;
	test.Start(_L("Starting tests... T_FSCHED"));
	parseCommandLine();
	
	TInt r = TheFs.Connect();
	TESTERROR(r);
	
	TDriveInfo info;
	TVolumeInfo volInfo;
	r = TheFs.Drive(info, gDrive);
	TESTERROR(r);
	
	if(info.iMediaAtt&KMediaAttVariableSize)
		{
		test.Printf(_L("Tests skipped in RAM drive\n"));
		goto out;
		}

	r = TheFs.Volume(volInfo, gDrive);
	if (r == KErrNotReady)
		{
		if (info.iType == EMediaNotPresent)
			test.Printf(_L("%c: Medium not present - cannot perform test.\n"), (TUint)gDrive + 'A');
		else
			test.Printf(_L("medium found (type %d) but drive %c: not ready\nPrevious test may have hung; else, check hardware.\n"), (TInt)info.iType, (TUint)gDrive + 'A');
		}
	else if (r == KErrCorrupt)
		{
		test.Printf(_L("%c: Media corruption; previous test may have aborted; else, check hardware\n"), (TUint)gDrive + 'A');
		}
	TESTERROR(r);
	
	if (!Is_Win32(TheFs, gDrive) && (volInfo.iDrive.iMediaAtt & KMediaAttFormattable))
		Format(gDrive);

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
