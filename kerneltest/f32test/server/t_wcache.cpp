// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_wcache.cpp
// This file contains a test for the Write Caching functionality of the File Server
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
#include <hal.h>
#include "tf32testtimer.h"

const TInt KTotalCacheSize = 32 * 1024 * 1024;
const TInt KDefaultCacheSize = (128 + 12) * 1024; 	// This size is the default configuration size
const TInt KFilesNeededToFillCache = (KTotalCacheSize / KDefaultCacheSize) + 2;
const TInt KMinSize = 254; // Boundary minim limit
const TInt KMaxSize = 257; // Boundary max limit



//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_WCACHE-0271
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ914
//! @SYMTestCaseDesc    This test case is exercising the Write Caching functionality added to 
//!						the File Server. There are negative and positive tests. 
//! @SYMTestActions     0   setup the environment to execute the tests
//!						1	TestBoundaries writes/reads around the write cache boundaries to
//!							the behaviour of the cache in some common cases. 
//!						2 	TestNegative ensures the integrity of data in the cache gets 
//!							preserved under error conditions
//!						3	TestIntegrity is trying to make sure integrity of the data is preserved
//!						4	TestFillCache fills the cache and then executes TestBoundaries. 
//!						5 	TestFillCacheNegative fills the cache with uncommitted data 
//!
//! @SYMTestExpectedResults finishes if the read cache behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------


////////////////////////////////////////////////////////////
// Template functions encapsulating ControlIo magic
//
template <class C>
TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));

    TInt r = fs.ControlIo(drv, fkn, ptrC);

    return r;
}

RTest test(_L("T_WCACHE"));

RFs gTheFs;
TInt gDrive;
TFileName gSessionPath;
TChar gDriveToTest;	
TThreadId gMainThreadId;
TInt gManual = 0;

HBufC8* gBuf = NULL;
TPtr8 gBufReadPtr(NULL, 0);	
HBufC8* gBufSec = NULL;
TPtr8 gBufWritePtr(NULL, 0);	

const TInt KOneK = 1024;
const TInt KOneMeg = KOneK * 1024;
const TInt KBlockSize = KOneK;
const TInt KWaitRequestsTableSize = 256;

TInt gSecondFileSize = 0; 
TInt gFirstFileSize = 0;

TInt64 gMediaSize = 0;

TTimeIntervalMicroSeconds gTimeTakenBigFile(0);
TBuf16<25> gFirstFile;
TBuf16<25> gSecondFile;
TBuf16<25> gCurrentFile;

TInt gNextFile = 0;

// Concurrent Threads
RThread gThread1;
RSemaphore gClient;
const TInt KHeapSize = 0x4000;
const TInt KMaxHeapSize = 0x100000;


/** Formats the drive 

	@param aDrive 	Drive to be formatted
	@param aFormatMode Mode for the format operation
*/
void Formatting(TInt aDrive, TUint aFormatMode )
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf = _L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r = format.Open(gTheFs,driveBuf,aFormatMode,count);
	test_KErrNone(r);
	while(count)
		{
		TInt r = format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	
	}

/** Verifies the content of a buffer 
	This function returns KErrNone when all the letters are consecutive in the aBuffer, KErrCorrupt otherwise

	@param aBuffer  Buffer to be verified
	
	@return KErrNone if all the letters are the same, KErrCorrupt otherwise
*/
TInt VerifyBuffer(TDes8& aBuffer)
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

/**  Fills a buffer with character aC, aC+1, aC+2, ..., aC+0x20, aC, etc 

	@param aBuffer  Buffer to be filled, output
	@param aLength  Length to be filled
	@param aC		Character to be used to fill the buffer
*/
void FillBuffer(TDes8& aBuffer, TInt aLength, TChar aC)
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
	
	@return ETrue if FAT, EFalse otherwise
*/
TBool IsFSFAT(RFs &aFsSession,TInt aDrive)
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
void FileNameGen(TDes16& aBuffer, TInt aLong, TInt aPos) 
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
TInt DeleteAllL(TDes16& aDir) 
	{
	TBuf16<100> dir;
	CFileMan* fMan = CFileMan::NewL(gTheFs);
	TInt r=0;
	
	dir = aDir;
	dir.Append(_L("F*.*"));
	r = fMan->Delete(dir);	

	delete fMan;
	return r;
	}

/**  Waits for all the TRequestStatus in status[] to complete

	@param status 	Array of TRequestStatus 
	@param aSize  Length to be filled
*/
void WaitForAll(TRequestStatus* status, TInt aSize) 
	{
	TInt i = 0;

	RTest test(_L("T_WCACHE"));

	while(i < aSize)
		{
		User::WaitForRequest(status[i]);
		if (status[i] != KErrNone)
			{
			test.Printf(_L("status[%d] == %d\n"), i, status[i].Int());
			test(EFalse);
			}
		i++;
		}

	test.Close();
	}

/**  Reads the parameters from the comand line 
	 and updates the appropriate variables
*/
void parseCommandLine() 
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
		
	r = gTheFs.CharToDrive(gDriveToTest,gDrive);
	test_KErrNone(r);
	
	if(!lex.Eos()) 
		{
		token.Set(lex.NextToken());	
		if(token.Length() != 0)		
			{
				TChar c = token[0];
				c.UpperCase();
				
				gManual = (c == 'M'); 
			}
		}
	
	gSessionPath = _L("?:\\F32-TST\\");
	gSessionPath[0] = (TUint16) gDriveToTest;
	test.Printf(_L("\nCLP=%C\n"),(TInt)gDriveToTest);
	}



/**  Writes a file synchronously in blocks of aBlockSize size

	@param aFs			RFs object
	@param aFile		File 
	@param aFileName	File name
	@param aSize		Size of the file in bytes
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aBuf			Buffer to be used to write
	@param aMode		Mode in which the file is meant to be opened
						
	@return Returns KErrNone if everything ok, otherwise it panics 
*/
TInt WriteFile(RFs& aFs, RFile& aFile, TDes16& aFileName, TInt aSize, TInt aBlockSize, TDes8& aBuf, TInt aMode) 
	{
	RTest test(_L("T_WCACHE"));

	TInt r = 0;

	test(aBlockSize > 0);	


	r = aFile.Replace(aFs,aFileName,aMode);
	test_KErrNone(r);

	TInt j = 0;
	while(j < aSize)
		{
			r = aFile.Write(aBuf, aBlockSize);
			test_KErrNone(r);
			j += aBlockSize;
		}					

	test.Close();
	return KErrNone;	
	}

/** Write a file that fits in the cache, and dies without proper cleaning

*/
LOCAL_C TInt WriteFileT(TAny* )
	{
	RTest test(_L("T_WCACHE"));
	RFs fs;
	RFile file;
	TInt r = fs.Connect();
	test_KErrNone(r);

	r = fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	
	r = WriteFile(fs, file, gFirstFile, KMinSize * KOneK, KBlockSize, gBufWritePtr, EFileShareAny|EFileWrite|EFileWriteBuffered);
	test_KErrNone(r);
	
	gClient.Signal();


	FOREVER
		{
			// waiting for the kill
		}		
	}

/**  Read File in blocks of size aBlockSize

	@param aFs			RFs object
	@param aFile		File 
	@param aFileName	File name
	@param aSize		Expected file size
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aMode		Mode in which the file is meant to be opened
						
	@return Returns KErrNone if everything ok, otherwise it panics 
*/
TInt ReadFile(RFs& aFs, RFile& aFile, TDes16& aFileName, TInt aSize, TInt aBlockSize, TInt aMode) 
	{
	RTest test(_L("T_WCACHE"));

	TInt r = 0, size = 0;
	
	test(aBlockSize>0);				// Block size must be greater than 0

	r = aFile.Open(aFs,aFileName,aMode);
	test_KErrNone(r);

	// Make sure the size of the file is the right one at this stage
	r = aFile.Size(size);
	test.Printf(_L("size of the file: %d \n"), size/KOneK); 
	test(size == aSize);
	
	TInt j = 0;
	while(j < size) 
	{
		r = aFile.Read(gBufReadPtr, aBlockSize);
		test_KErrNone(r);
		j += aBlockSize;
	}					

	test.Close();
	return KErrNone;	
	}

/** Write a file asynchronously in blocks of aBlockSize size

	@param aFs			RFs object
	@param aFileWrite	RFile object, needs to exist beyond the scope of this function
	@param aFile		File name
	@param aSize		Size of the file in bytes
	@param aMode		Specifies the mode in which the file should be openned
	@param aStatus		TRequestStatus array for all the requests
*/
void WriteFileAsync(RFs& aFs, RFile& aFileWrite, TDes16& aFile, TInt aSize, TInt aMode, TRequestStatus aStatus[]) 
	{
	RTest test(_L("T_WCACHE"));

	TInt r = 0;
	
	r = aFileWrite.Replace(aFs,aFile,aMode);
	test_KErrNone(r);

	TInt j = 0, i = 0;
	while(j < aSize)
		{
		aFileWrite.Write(gBufWritePtr, KBlockSize, aStatus[i]);
		r = aStatus[i].Int();
		if (r != KErrNone && r != KRequestPending)
			{
			test.Printf(_L("Write %d returned %d\n"), i, r);
			test(0);
			}
		i++;

		j += KBlockSize;	
		}					
	test.Close();
	}

/**  Read a file asynchronously in blocks of aBlockSize size

	@param aFs			RFs object
	@param aFileRead	RFile object, needs to exist beyond the scope of this function
	@param aFile		File name
	@param aFileSize		Size of the file in bytes
	@param aBlockSize	Size of the blocks to be used in bytes
	@param aStatus		TRequestStatus array for all the requests
	@param aMode		Specifies the mode in which the file should be openned
	
	@return KErrNone
*/
TInt ReadFileAsync(RFs& aFs,RFile& aFileRead, TDes16& aFile, TInt aFileSize, TInt aBlockSize,TRequestStatus aStatus[],  TInt aMode) 
	{
	RTest test(_L("T_WCACHE"));

	TInt r = 0;
	TInt size = 0;
	
	test(aBlockSize > 0);			
	

	r = aFileRead.Open(aFs,aFile, aMode); 
	test_KErrNone(r);
	
	r = aFileRead.Size(size);
	test_KErrNone(r);

	test.Printf(_L("size of the file %d\n"), size/KOneK);
	test(size == aFileSize);
	
	TInt j = 0, i = 0;
	while(j < size) 
		{
		aFileRead.Read(gBufReadPtr, aBlockSize, aStatus[i]);
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

	@param aFile 	 	File object
	@param aFileName 	File Name
	@param aSize 		Size in kilobytes
	@param aBlockSize 	Size of the block
	@param aMode 		Mode in which the file is going to be opened
	
	@return time taken to perform the operation in uS
*/
TTimeIntervalMicroSeconds WriteTestFile(RFile& aFile, TDes16& aFileName, TInt aSize, TInt aBlockSize, TInt aMode) 
	{
	RTest test(_L("T_WCACHE"));

	TInt r = 0;

    TF32TestTimer timer;
	timer.Start();
	
	r = WriteFile(gTheFs,aFile, aFileName , aSize * KOneK, aBlockSize, gBufWritePtr, aMode);
	test_KErrNone(r);

	timer.Stop();	
	gTimeTakenBigFile = timer.Time();
	test.Close();
	return timer.Time();        
	}

/** Measure the time taken for this file to be read synchronously

	@param aFile 	 	File object
	@param aFileName 	File Name
	@param aSize 		Size in kilobytes
	@param aBlockSize 	Size of the block
	@param aMode 		Mode in which the file is going to be opened

	@return time taken to perform the operation in uS

*/
TTimeIntervalMicroSeconds ReadTestFile(RFile& aFile, TDes16& aFileName, TInt aSize, TInt aBlockSize, TInt aMode) 
	{
    TF32TestTimer timer;
	timer.Start();
	ReadFile(gTheFs,aFile, aFileName, aSize * KOneK, aBlockSize, aMode);
	timer.Stop();
	
	gTimeTakenBigFile = timer.Time();
        	
	return timer.Time();
	}

/** Read asynchronously the test file from the disc

	@param aFile 	 	File object
	@param aFileName 	File Name
	@param aSize 		Size in kilobytes
	@param aBlockSize 	Size of the block
	@param aMode 		Mode in which the file is going to be opened

	@return time taken to perform the operation in uS
*/
TTimeIntervalMicroSeconds ReadAsyncTestFile(RFile& file, TDes16& aFile, TInt aSize, TInt aBlockSize, TInt aMode) 
	{
	TRequestStatus status[KWaitRequestsTableSize];
    TF32TestTimer timer;
	timer.Start();
	
	ReadFileAsync(gTheFs, file, aFile, aSize * KOneK, aBlockSize, status, aMode);
	WaitForAll(status,  (aSize * KOneK)/KBlockSize);
	timer.Stop();	
		
	gTimeTakenBigFile = timer.Time();
	
	return timer.Time();
	}

/** Read asynchronously the test file from the disc

	@param aFile 	 	File object
	@param aFileName 	File Name
	@param aSize 		Size in kilobytes
	@param aMode 		Mode in which the file is going to be opened

	@return time taken to perform the operation in uS
*/
TTimeIntervalMicroSeconds WriteAsyncTestFile(RFile& aFile, TDes16& aFileName, TInt aSize, TInt aMode) 
	{
	TRequestStatus status[KWaitRequestsTableSize];
    TF32TestTimer timer;
    timer.Start();
	
	WriteFileAsync(gTheFs, aFile, aFileName, aSize * KOneK, aMode, status );
	WaitForAll(status, (aSize * KOneK)/KBlockSize);
	timer.Stop();	
	
	gTimeTakenBigFile = timer.Time();
	
	return timer.Time();
	}

/**  Test Boundaries

	This function is testing the behaviour on the boundaries of the write cache size
*/
void TestBoundaries()
	{
	TInt r = 0;
	TTimeIntervalMicroSeconds time = 0;
	TTimeIntervalMicroSeconds rtime = 0;
	TTimeIntervalMicroSeconds tcreate = 0;
	RFile fileWriter;
	RFile fileWriter2;
	RFile fileReader;

	test.Start(_L("Test Boundaries"));
	
	// Test boundaries from 254K to 256K, synchronous operations 
	TInt i = KMinSize;
	
	
	test.Printf(_L("\n\n\n"));
	
	while(i < KMaxSize) 
		{
		test.Printf(_L("\nSync: Write from 1 K to %d K \n"), i); 

		tcreate = WriteTestFile(fileWriter, gSecondFile, i, KBlockSize, EFileShareAny|EFileWrite|EFileWriteDirectIO);
		test.Printf(_L("Time to write %d K without caching: %d mS\n"), i, TF32TestTimer::TimeInMilliSeconds(tcreate));	
		fileWriter.Close();

		time =  WriteTestFile(fileWriter2, gFirstFile, i, KBlockSize, EFileShareAny|EFileWrite|EFileWriteBuffered);
		test.Printf(_L("Time to write %d K WITH caching: %d mS\n"), i, TF32TestTimer::TimeInMilliSeconds(time));

		rtime = ReadTestFile(fileReader, gFirstFile, i, KBlockSize, EFileShareAny|EFileRead|EFileReadBuffered);
		test.Printf(_L("Time to read %d K from the cache: %d mS\n"), i, TF32TestTimer::TimeInMilliSeconds(rtime));

		fileReader.Close();	
		fileWriter2.Close();
		
		#if !defined(__WINS__)
            test(tcreate > TTimeIntervalMicroSeconds(0));  // test measured time is correct
			test(tcreate > time);
            test(tcreate > rtime); 
		#endif

		r = gTheFs.Delete(gFirstFile);
		test_KErrNone(r);
		r = gTheFs.Delete(gSecondFile);
		test_KErrNone(r);
		
		i++;
		}

	test.Printf(_L("\n\n\n"));
	
	// Test boundaries from 254K to 256K, asynchronous operations 
	i = KMinSize;
	
	while(i < KMaxSize) 
		{
		test.Printf(_L("\nAsync: Write from 1 K to %d K \n"), i); 

		tcreate = WriteAsyncTestFile(fileWriter, gSecondFile, i, EFileShareAny|EFileWrite|EFileWriteDirectIO);
		test.Printf(_L("Time to write %d K without caching: %d mS\n"), i, TF32TestTimer::TimeInMilliSeconds(tcreate));
		fileWriter.Close();

		time =  WriteAsyncTestFile(fileWriter2, gFirstFile, i,EFileShareAny|EFileWrite|EFileWriteBuffered);
		test.Printf(_L("Time to write %d K WITH caching: %d mS\n"), i, TF32TestTimer::TimeInMilliSeconds(time));


		rtime = ReadAsyncTestFile(fileReader, gFirstFile, i, KBlockSize, EFileShareAny|EFileRead|EFileReadBuffered);
		test.Printf(_L("Time to read %d K from the cache: %d mS\n"), i, TF32TestTimer::TimeInMilliSeconds(rtime));

		fileReader.Close();	
		fileWriter2.Close();
		
		#if !defined(__WINS__)
            test(tcreate > TTimeIntervalMicroSeconds(0));  // test measured time is correct
			test(tcreate > time);
            test(tcreate > rtime);
		#endif

		r = gTheFs.Delete(gFirstFile);
		test_KErrNone(r);
		r = gTheFs.Delete(gSecondFile);
		test_KErrNone(r);
		
		i++;
		}

	test.End();
	}

/**  Test negative cases

*/
void TestNegative()
	{
	TInt r = 0;
	RFile file;
	TInt size =0;
		
	TBuf<20> buf = _L("Write File");


	test.Start(_L("Test Negative"));

	test.Next(_L("Kill a simple operation"));
	
	r = gThread1.Create(buf,WriteFileT,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread1.Resume();
	gClient.Wait();

	gThread1.Kill(KErrGeneral);

	r = file.Open(gTheFs,gFirstFile,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
	test_KErrNone(r);

	r = file.Size(size);
	test_KErrNone(r);
	
	test.Printf(_L("The size of the file is %d KB\n\n"), size/KOneK);
	test(size == (KMinSize * KOneK));
	
	file.Close();

	test.End();
	}


/** Read the file verifying content

	@param aFile file name to verify 
	
	@return returns the time that took to do the verification in mS, fails if the file is not corrupted/modified
*/
TTimeIntervalMicroSeconds ReadTestFileVerif(TDes16& aFile)
	{
	TInt r = 0;
	TInt size = 0;
	RFile fileRead;
	TInt corrupt = 0;
	TBool isFat=IsFSFAT(gTheFs,gDrive);

    TF32TestTimer timer;
	timer.Start();
	
	r = fileRead.Open(gTheFs,aFile,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
	test_KErrNone(r);

	r = fileRead.Size(size);
	test_KErrNone(r);
	
	TInt j = 0;
	
	while(j < size) 
		{
			r = fileRead.Read(gBufReadPtr, KBlockSize);
			if(isFat)
			{
				test_KErrNone(r);
			}
			else 
			{
			if(r == KErrCorrupt) 
				corrupt++;
			}
			
			j += KBlockSize;
			r = VerifyBuffer(gBufReadPtr);
			if(r == KErrCorrupt) 
				corrupt++;
		}					

	fileRead.Close();
	
	test(corrupt>0); // Ensure the cache returns the changed content 
	timer.Stop();	
	
	gTimeTakenBigFile = timer.Time();
	
	return timer.Time();
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
	test_Value(res, res == KErrNone && dummy != NULL);
		
	dummyPtr.Set(dummy->Des());
	FillBuffer(dummyPtr, 4, '1');

	r = fileWrite.Open(gTheFs,gSecondFile,EFileShareAny|EFileWrite|EFileWriteBuffered);
	if(r != KErrNone) 
		return r;
	TInt pos = 30;
	r = fileWrite.Seek(ESeekStart,pos);
	test_KErrNone(r);
	
	r = fileWrite.Write(dummyPtr);
	if(r != KErrNone) 
		return r;
	
	fileWrite.Close();

	delete dummy;

	return KErrNone;
	}


/** Integrity testing

*/
LOCAL_C void TestIntegrity()
	{
	TInt r = 0;
	TTimeIntervalMicroSeconds time;
	TTimeIntervalMicroSeconds tcreate = 0;
	RFile file;
	
	// Modify file in some position 
	test.Printf(_L("Overwrite partially a file\n"));
	
	test.Printf(_L("\nSync: Write from 1 K to %d K \n"), 255); 

	tcreate = WriteTestFile(file, gSecondFile, 255, KBlockSize, EFileShareAny|EFileWrite|EFileWriteBuffered);
	test.Printf(_L("Time to write %d K with caching: %d mS\n"), 255, TF32TestTimer::TimeInMilliSeconds(tcreate));	
	file.Close();

	test.Printf(_L("Mess the content that is still in the cache\n"));
	CorruptSecondFile(); 
	
	time = ReadTestFileVerif(gSecondFile);	
	test.Printf(_L("Time taken to verify: %ld\n"),time.Int64());
	
	test.Printf(_L("Integrity verified\n"));

	r = DeleteAllL(gSessionPath);
	test_KErrNone(r);
	}

/**  Creates the files to fill the cache with dirty data
	
	@return KErrNone
*/
TInt CreateFilesThread(TAny *)
	{
	TInt i = 0;
	TInt r = 0;
	TBuf16<50> directory;
	
	TBuf16<50> path;
	TBuf16<50> buffer(50); 	
	RFile file[KFilesNeededToFillCache];
	
	RTest test(_L("T_WCACHE2"));
	RFs fs;
	
	fs.Connect();
	
	directory = gSessionPath;
	
	test.Printf(_L("Creating %d files for filling the cache (size %d)\n"), KFilesNeededToFillCache, KDefaultCacheSize);

	// create a big buffer to speed things up
	HBufC8* bigBuf = NULL;
	TInt KBigBifferSize = 32 * KOneK;
	
	TRAPD(res,bigBuf = HBufC8::NewL(KBigBifferSize));
	test_Value(res, res == KErrNone && bigBuf != NULL);
		
	TPtr8 bigBufWritePtr(NULL, 0);	
	bigBufWritePtr.Set(bigBuf->Des());
	FillBuffer(bigBufWritePtr, KBigBifferSize, 'A');
	
	i = 0;		
	while(i < KFilesNeededToFillCache) 
		{
		if (i % 10 == 0)
			test.Printf(_L("Creating file %d of %d...\r"), i, KFilesNeededToFillCache);
		FileNameGen(buffer, 8, i+3) ;
		path = directory;
		path.Append(buffer);

		r = file[i].Create(fs,path,EFileShareAny|EFileWrite|EFileWriteBuffered);
		if(r == KErrAlreadyExists) 
			r = file[i].Open(fs,path,EFileShareAny|EFileWrite|EFileWriteBuffered);
		test_KErrNone(r);
		TInt j = 0;
	
		while(j < KDefaultCacheSize)
			{
			bigBufWritePtr.SetLength(Min(KBigBifferSize, KDefaultCacheSize - j));
			
			r = file[i].Write(bigBufWritePtr);
			test_KErrNone(r);
			j += bigBufWritePtr.Length();
			}					

		// Not closing the files is done on purpose, as part of the test

		i++;
		}
	test.Printf(_L("\nFiles created\n"));
	delete bigBuf;
	
	gClient.Signal();
	
	return KErrNone;
	}


/**  Creates the files to fill the read cache 

	@param aFiles 	 Number of files needed to fill the cache
	@param aFileSize The file size
*/
void CreateFiles(TInt aFiles, TInt aFileSize)
	{
	TInt i = 0;
	TInt r = 0;
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
	test_Value(res, res == KErrNone && bigBuf != NULL);
		
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

		// delete file first to ensure it's contents are not in the cache (file may be be on the closed file queue)
		r = gTheFs.Delete(path);
		test_Value(r, r == KErrNone || r == KErrNotFound);

		r = file.Create(gTheFs,path,EFileShareAny|EFileWrite|EFileWriteDirectIO);
		if(r == KErrAlreadyExists) 
			r = file.Open(gTheFs,path,EFileShareAny|EFileWrite|EFileWriteDirectIO);
		test_KErrNone(r);
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
void FillCache(RFile aFile[KFilesNeededToFillCache], TInt aFiles, TInt aFileSize)
	{
	TInt i = 0;
	TInt r = 0;
	TBuf16<50> directory;
	
	TBuf16<50> path;
	TBuf16<50> buffer(50); 	
	HBufC8* buf = NULL;
	TPtr8 bufPtr(NULL, 0);	
	
	TRAPD(res,buf = HBufC8::NewL(2));
	test_Value(res, res == KErrNone && buf != NULL);
	bufPtr.Set(buf->Des());
	
	directory = gSessionPath;
	
	i = 0;		
	while(i < aFiles) 
	{
		FileNameGen(buffer, 8, i+3) ;
		path = directory;
		path.Append(buffer);
		r = aFile[i].Open(gTheFs,path,EFileShareAny|EFileRead|EFileReadBuffered|EFileReadAheadOff);
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
void TestFillCache()
	{	
	TInt nFiles = KFilesNeededToFillCache;
	TInt fSize = KDefaultCacheSize;
	RFile file[KFilesNeededToFillCache];
	TInt r = 0;
	
	if(gMediaSize> ((fSize * nFiles)+gSecondFileSize+gFirstFileSize))
		{
		test.Start(_L("Creating files for filling the cache\n"));
		CreateFiles(nFiles,fSize);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		TFileCacheStats startPageCacheStats;
		r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list at beginning=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list at beginning=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
#endif
		FillCache(file,nFiles,fSize); 

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list at end=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list at end=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
#endif

	TestBoundaries();
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list after the boundary testing=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list after the boundary testing=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
#endif

		TInt i = 0;
		while( i < KFilesNeededToFillCache )
			{
			file[i++].Close();
			}
			
		r = DeleteAllL(gSessionPath);
		test_KErrNone(r);

		test.End();
		}
	else 
		test.Printf(_L("Skipping the fill of the cache due to lack of space in the current drive\n"));
	}

/** Fills the cache and generate error situations

*/
void TestFillCacheNegative()
	{	
	TInt nFiles = KFilesNeededToFillCache;
	TInt r = 0;

	if(gMediaSize> ((KDefaultCacheSize * nFiles)+gSecondFileSize+gFirstFileSize))
		{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		TFileCacheStats startPageCacheStats;
		r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list at beginning=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list at beginning=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
#endif
	test.Start(_L("Creating files for filling the cache, with uncommitted data\n"));
	
	TBuf<20> buf = _L("FillCache");
	
	 r = gThread1.Create(buf,CreateFilesThread,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gThread1.Resume();
	gClient.Wait();

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list at end=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list at end=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
#endif

	TestBoundaries();
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
		// get number of items on Page Cache
		r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list after the boundary testing=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list after the boundary testing=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);
		
		User::After(180000);

		r = controlIo(gTheFs,gDrive, KControlIoFileCacheStats, startPageCacheStats);
		test_Value(r, r == KErrNone || r == KErrNotSupported);
		test.Printf(_L("Number of page cache lines on free list after the boundary testing=%d\n"),startPageCacheStats.iFreeCount);
		test.Printf(_L("Number of page cache lines on used list after the boundary testing=%d\n"),startPageCacheStats.iUsedCount);
		test.Printf(_L("Number of files on closed queue=%d\n"),startPageCacheStats.iFilesOnClosedQueue);

#endif
		test.End();
		
		r = DeleteAllL(gSessionPath);
		test_KErrNone(r);

		}
	else 
		test.Printf(_L("Skipping the fill of the cache due to lack of space in the current drive\n"));
	}


/** Manual test for card removal

*/
void TestRemoval()
	{	
	TTimeIntervalMicroSeconds time = 0, rtime = 0;
	RFile file1, file2;
	
		 	
	TInt r = gClient.CreateLocal(0);
 	test_KErrNone(r);
 	
	r = gTheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	
	r = gTheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	test.Printf(_L("Disabling Lock Fail simulation ...\n"));
	// turn OFF lock failure mode
	TBool simulatelockFailureMode = EFalse;
	r = controlIo(gTheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_KErrNone(r);
#endif

	TBuf16<45> dir;
		
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
	test_Value(res, res == KErrNone && gBuf != NULL);
		
	gBufWritePtr.Set(gBuf->Des());
	FillBuffer(gBufWritePtr, KBlockSize, 'A');
	
	TRAPD(res2,gBufSec = HBufC8::NewL(KBlockSize+1));
	test(res2 == KErrNone && gBufSec != NULL);
	gBufReadPtr.Set(gBufSec->Des());

	
	test.Printf(_L("\nSync: Write from 1 K to 254 K \n")); 

	time =  WriteTestFile(file1, gSecondFile, KMinSize, KBlockSize, EFileShareAny|EFileWrite|EFileWriteBuffered);
	test.Printf(_L("Time to write %d K WITH caching: %d mS\n"), KMinSize, TF32TestTimer::TimeInMilliSeconds(time));
	test.Printf(_L("Remove MMC card,! and then press a key\n"));
	test.Getch();

	test.Printf(_L("Wait 3 seconds and insert MMC card! and then press a key\n"));
	test.Getch();

	rtime = ReadTestFile(file2, gSecondFile, KMinSize, KBlockSize, EFileShareAny|EFileRead|EFileReadBuffered);
	test.Printf(_L("Time to read %d K from the cache: %d mS\n"), KMinSize, TF32TestTimer::TimeInMilliSeconds(rtime));

	test.Printf(_L("Remove MMC card! and then press a key\n"));
	test.Getch();

	test.Printf(_L("Wait 3 seconds and insert MMC card! and then press a key\n"));
	test.Getch();


	test.Printf(_L("\nSync: Write from 1 K to 255 K \n")); 

	time =  WriteTestFile(file1, gFirstFile, KMinSize + 1 , KBlockSize, EFileShareAny|EFileWrite|EFileWriteBuffered);
	test.Printf(_L("Time to write %d K WITH caching: %d mS\n"), KMinSize + 1, TF32TestTimer::TimeInMilliSeconds(time));
	test.Printf(_L("Remove MMC card and delete the file //F32-TST//FFFFFFF0.TXT and then press a key\n"));
	test.Getch();

	test.Printf(_L("Wait 3 seconds and insert MMC card! and then press a key\n"));
	test.Getch();

	rtime = ReadTestFile(file2, gFirstFile, KMinSize + 1, KBlockSize, EFileShareAny|EFileRead|EFileReadBuffered);
	test.Printf(_L("Time to read %d K from the cache: %d mS\n"), KMinSize + 1, TF32TestTimer::TimeInMilliSeconds(rtime));

	test.Printf(_L("Remove MMC card! and then press a key\n"));
	test.Getch();

	test.Printf(_L("Wait 3 seconds and insert MMC card! and then press a key\n"));
	test.Getch();
	
		
	file1.Close();	
	file2.Close();
	delete gBuf;
	delete gBufSec;
	}


/** Main tests function
*/ 
void CallTestsL()
	{
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	test.Printf(_L("Disabling Lock Fail simulation ...\n"));
	// turn OFF lock failure mode
	TBool simulatelockFailureMode = EFalse;
	TInt r = controlIo(gTheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_KErrNone(r);
#endif

	TBuf16<45> dir;
		
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
	test_Value(res, res == KErrNone && gBuf != NULL);
		
	gBufWritePtr.Set(gBuf->Des());
	FillBuffer(gBufWritePtr, KBlockSize, 'A');
	
	TRAPD(res2,gBufSec = HBufC8::NewL(KBlockSize+1));
	test(res2 == KErrNone && gBufSec != NULL);
	gBufReadPtr.Set(gBufSec->Des());

	test.Next(_L("Boundary test"));
	TestBoundaries();
	
	test.Next(_L("Negative test\n"));
	TestNegative(); 

	test.Next(_L("Integrity test\n"));
	TestIntegrity(); 
	

	test.Next(_L("Fill the cache, boundary testing\n"));
	TestFillCache();
	
	test.Next(_L("Fill the cache negative, boundary testing\n"));
	TestFillCacheNegative();
	
	test.End();
	delete gBuf;
	delete gBufSec;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// turn lock failure mode back ON (if enabled)
	simulatelockFailureMode = ETrue;
	r = controlIo(gTheFs, gDrive, KControlIoSimulateLockFailureMode, simulatelockFailureMode);
	test_KErrNone(r);
#endif

	}

/** Initialises semaphores and call the tests
*/ 
void DoTests()
	{
	TInt r = 0;
	 	
	r = gClient.CreateLocal(0);
 	test_KErrNone(r);
 	
	r = gTheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	
	r = gTheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);
	gTheFs.ResourceCountMarkStart();
	
	TRAP(r,CallTestsL());
	
	test_KErrNone(r);
	gTheFs.ResourceCountMarkEnd();
	}

/** Determines the space that can be used for the files

*/
TBool CheckForDiskSize()
	{
	TVolumeInfo volInfo;
	TInt r = gTheFs.Volume(volInfo, gDrive);
	test_KErrNone(r);
	gMediaSize = volInfo.iFree;	
	test.Printf(_L("\nMedia free space: %d MB\n"), gMediaSize/KOneMeg);
	return ETrue;
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
	test.Start(_L("Starting tests... T_WCACHE"));
	parseCommandLine();
	
	TInt r = gTheFs.Connect();
	test_KErrNone(r);
	
	TDriveInfo info;
	TVolumeInfo volInfo;
	r = gTheFs.Drive(info,gDrive);
	test_KErrNone(r);
	
	if(info.iMediaAtt&KMediaAttVariableSize)
		{
		test.Printf(_L("Tests skipped in RAM drive\n"));
		goto out;
		}

	r = gTheFs.Volume(volInfo, gDrive);
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

	if(!CheckForDiskSize())
		{
		test.Printf(_L("Skipping tests due to lack of space to perform them in this drive\n"));
		}
	else if(!gManual)
		{
		DoTests();
		}
	else
		{
		TestRemoval();
		}

out:	
	test.End();

	gTheFs.Close();
	test.Close();

	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }

