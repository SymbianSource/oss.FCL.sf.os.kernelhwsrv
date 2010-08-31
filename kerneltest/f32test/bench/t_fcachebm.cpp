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
// f32test/bench/t_fcachebm.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32math.h>
#include "t_select.h"
#include "t_benchmain.h"

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_RCACHE-0192
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ914
//! @SYMTestCaseDesc    This test case is testing performance of the File Server Cache.  
//! @SYMTestActions     0   setup the environment to execute the tests
//!						1	small random reads/writes
//!						2	large sequential reads/writes
//!						3 	streaming test (100, 200 and 500 kbps)
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------


GLDEF_D RTest test(_L("File Cache BM"));

LOCAL_D RSemaphore client;
LOCAL_D const TInt KHeapSize = 0x4000;
LOCAL_D const TInt KTobps = 1000;
LOCAL_D const TInt KByte = 8;
LOCAL_D const TInt KOneSec = 1000000; // One second in microseconds

// Tests setup
LOCAL_D const TInt KSmallRow = 11;
LOCAL_D const TInt KSmallCol = 7;
LOCAL_D const TInt KSmallThreshold = 64;
LOCAL_D const TInt KSeveralTimes = 10;

LOCAL_D const TInt KLargeRow = 19;
LOCAL_D const TInt KLargeCol = 3;

LOCAL_D TInt gCurrentSpeed = 0; 
LOCAL_D TInt gCurrentBS = 0;


LOCAL_D TBuf8<4096> buf;
LOCAL_D TDriveList gDriveList;

// Concurrent thread
RThread gSpeedy;
RThread gSpeedyII;

TBuf16<25> gBaseFile;
TBuf16<25> gFileA;
TBuf16<25> gFileB;
TBuf16<25> gStreamFile;

HBufC8* gBuf = NULL;
TPtr8 gBufReadPtr(NULL, 0);	
HBufC8* gBufSec = NULL;
TPtr8 gBufWritePtr(NULL, 0);	


HBufC8* gBufB = NULL;
TPtr8 gBufReadBPtr(NULL, 0);	
TBool gWriting = EFalse;

LOCAL_D TInt ThreadCount=0;

enum TTestState 
	{
	ENoThreads,			// No threads 
	ETwoThreadsDif,		// Accessing to different files 
	ETwoThreadsDifDif, 	// Accessing to different files, different blocksizes
	ETwoThreadsSame		// Accessing the same file
	};


/** 2 ^ b

	@param b Power 
*/
LOCAL_C TInt Pow(TInt b)
{
	return 1 << b;
}

/** Pseudo-random number generator, random enough for the purpose

	@param aMax Upper limit of the number. The interval of generated numbers is [0,aMax)
*/
LOCAL_C TInt Rand(TInt aMax) 
{
	return (Math::Random() % aMax);
}


/** Fills a buffer with character aC, aC+1, aC+2, ..., aC+32, aC, etc 

	@param aBuffer 	Buffer to fill out
	@param aLength 	Length to be filled with characters
	@param aC 		Base character to fill the buffer
*/
LOCAL_C void FillBuffer(TDes8& aBuffer, TInt aLength, TChar aC)
	{
	test (aBuffer.MaxLength() >= aLength);
	
	for(TInt i = 0; i < aLength; i++)
		{
		aBuffer.Append((i%32)+aC);
		}
	}
	
/** Send content through the RDebug for trgtest 
	not to hung, when the test is not writing

*/
LOCAL_C TInt noise(TAny* )
{
	FOREVER
	{	
		User::After(2147483647); // max value, 35 minutes, 47 seconds
		if(!gWriting)
			RDebug::Print(_L("."));
	}
}

/**  Delete content of directory

	@param aDir Directory to be emptied
*/
LOCAL_C TInt DeleteAll(TDes16& aDir) 
	{
		TBuf16<100> dir;
		CFileMan* fMan = CFileMan::NewL(TheFs);
		TInt r = 0;
		
		dir = aDir;
		dir.Append(_L("F*.*"));
		r = fMan->Delete(dir);	

		delete fMan;
		return r;
	}


/**   Creates a file of aSize KBytes

	@param aFile File name 
	@param aSize Size of the file to be created
*/
LOCAL_C void CreateFile(TDes16& aFile, TInt aSize)
{
	TInt r = 0;
	RFile file;
	
	
	r = file.Replace(TheFs, aFile, EFileShareAny|EFileWrite);
	FailIfError(r);

	TInt j = 0;
	while(j <= aSize)
		{
			r = file.Write(gBufWritePtr, KOneK);
			FailIfError(r);
			j += KOneK;
		}					
	file.Close();
}

/**   Kills the concurrent session

*/
LOCAL_C void DoTestKill()
{
	TInt r = 0;
	
	gSpeedy.Kill(KErrNone);
	FailIfError(r);
	gSpeedy.Close();	
	
	gSpeedyII.Kill(KErrNone);
	FailIfError(r);
	gSpeedyII.Close();	
}

/** Read aSize KBs from aFile file in blocks of aBlockSize bytes

	@param aFile 		Name of the file
	@param aPos			Position from where the read starts within the file
	@param aBlockSize 	Block size for the I/O operation
	@param aSize 		
*/
LOCAL_C TInt ReadFromFile(TDes16& aFile, TInt aPos, TInt aBlockSize, TInt aSize) 
{
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	RFile file;
	TInt j = 0;
	TInt size = aSize * KOneK;
	TTimeIntervalMicroSeconds timeTaken(0);

	r = file.Open(TheFs,aFile,EFileShareAny|EFileRead);
	FailIfError(r);
	
	startTime.HomeTime();	
	r = file.Seek(ESeekStart, aPos);
	FailIfError(r);	
	
	while(j <= size)
		{
			r = file.Read(gBufReadPtr, aBlockSize);
			FailIfError(r);
			j += aBlockSize;
		}					
	endTime.HomeTime();
	
	file.Close();
	timeTaken = endTime.MicroSecondsFrom(startTime);
	
	return I64LOW(timeTaken.Int64() / gTimeUnit);
}

/** Read aSize KBs from aFile file in blocks of aBlockSize bytes several times

	@param aFile 		Name of the file
	@param ayMax 		Maximum for the position
	@param aBlockSize 	Block size for the I/O operation
	@param aSize 		Size of the file in KB
*/
LOCAL_C TInt ReadFromFileSeveralTimes(TDes16& aFile, TInt ayMax, TInt aBlockSize, TInt aSize) 
{
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	RFile file;
	TInt j = 0, i, pos;
	TInt size = aSize * KOneK;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt64 time = 0;
	

	r = file.Open(TheFs, aFile, EFileShareAny|EFileRead);
	FailIfError(r);
	
	i = 0;
	while( i < KSeveralTimes ) 
		{
		pos = Rand(Pow(ayMax - 1));
		startTime.HomeTime();	
		r = file.Seek(ESeekStart, pos); 
		FailIfError(r);	
		
		j = 0;
		while(j <= size)
			{
				r = file.Read(gBufReadPtr, aBlockSize);
				FailIfError(r);
				j += aBlockSize;
			}					
		endTime.HomeTime();

		timeTaken = endTime.MicroSecondsFrom(startTime);
		time += I64LOW(timeTaken.Int64());
		i++;
		}
	
	file.Close();
	
	return I64LOW((time / gTimeUnit) / KSeveralTimes); // Overflow not expected here, since the number is not big enough
}


/** Write aSize KBs to  aFile file in blocks of aBlockSize bytes

	@param aFile 		Name of the file
	@param aPos			Position from where the read starts within the file
	@param aBlockSize 	Block size for the I/O operation
	@param aSize 		Size of the file in KB
*/
LOCAL_C TInt WriteToFile(TDes16& aFile, TInt aPos, TInt aBlockSize, TInt aSize) 
{
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	RFile file;
	TInt j = 0;
	TInt size = aSize * KOneK;
	TTimeIntervalMicroSeconds timeTaken(0);
	
	r = file.Open(TheFs, aFile, EFileShareAny|EFileWrite);
	FailIfError(r);
	
	startTime.HomeTime();	
	r = file.Seek(ESeekStart, aPos);
	FailIfError(r);	
	while(j <= size)
		{
			r = file.Write(gBufWritePtr, aBlockSize);
			FailIfError(r);
			j += aBlockSize;
		}					
	endTime.HomeTime();
	
	file.Close();
	timeTaken = endTime.MicroSecondsFrom(startTime);
	
	return I64LOW(timeTaken.Int64() / gTimeUnit);
}

/** Write aSize KBs to  aFile file in blocks of aBlockSize bytes several times

	@param aFile 		Name of the file
	@param ayMax 		Maximum for the position
	@param aBlockSize 	Block size for the I/O operation
	@param aSize 		Size of the file in KB
*/
LOCAL_C TInt WriteToFileSeveralTimes(TDes16& aFile, TInt ayMax, TInt aBlockSize, TInt aSize) 
{
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	RFile file;
	TInt i, j = 0, pos;
	TInt size = aSize * KOneK;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt64 time = 0;
	
	r = file.Open(TheFs, aFile, EFileShareAny|EFileWrite);
	FailIfError(r);
	
	i = 0;
	while( i < KSeveralTimes ) 
		{
		pos = Rand(Pow(ayMax - 1));
		startTime.HomeTime();	
		r = file.Seek(ESeekStart, pos);
		FailIfError(r);	
		
		j = 0;
		while(j <= size)
			{
				r = file.Write(gBufWritePtr, aBlockSize);
				FailIfError(r);
				j += aBlockSize;
			}					
		endTime.HomeTime();
	
		timeTaken = endTime.MicroSecondsFrom(startTime);
		time += I64LOW(timeTaken.Int64());
		i++;
		}
		
	file.Close();

	return I64LOW((time / gTimeUnit) / KSeveralTimes); // Overflow not expected here, since the number is not big enough
}

/** Read small blocks in the gBaseFile

*/
LOCAL_C TInt ReadSmallBase(TAny* )
{
	RTest test(_L("test 2"));
	RFs fs;
	RFile file;
	TBuf8<1024> dummy(1024);
	TInt blockSize = 0;
	TInt randPos = 0;
	TInt i = 0;

	TInt r = fs.Connect();
	
	fs.SetSessionPath(gSessionPath);
	r = file.Open(fs, gBaseFile, EFileShareAny|EFileRead);
	client.Signal();
	
	FailIfError(r);
		
	FOREVER
	{	
			randPos = Rand(64) * KOneK ; 
			r = file.Seek(ESeekStart, randPos);
			FailIfError(r);
			blockSize = i;
			
			r = file.Read(dummy, blockSize); // Sync operation
			if(i >= 1023) i = 0;
			else i++;
	}
}
	
/** Read small blocks in gFileA

*/
LOCAL_C TInt ReadSmallA(TAny* )
{
	RTest test(_L("test 2"));
	RFs fs;
	RFile file;
	TInt blockSize = 0;
	TInt randPos = 0;
	TBuf8<1024> dummy(1024);
	TInt i = 0;

	TInt r = fs.Connect();
	
	fs.SetSessionPath(gSessionPath);

	r = file.Open(fs,gFileA,EFileShareAny|EFileRead);
	
	client.Signal();
	FailIfError(r);
	
	FOREVER
	{	
			randPos = Rand(64) * KOneK ; 
			r = file.Seek(ESeekStart, randPos);
			FailIfError(r);
			
			blockSize = i;
			
			r = file.Read(dummy, blockSize); // Sync operation
			FailIfError(r);
			if(i >= 1023) i = 0;
			else i++;
	}
}
	
/** Read small blocks in gFileB

*/
LOCAL_C TInt ReadSmallB(TAny* )
	{
	RTest test(_L("test 2"));
	RFs fs;
	RFile file;
	TBuf8<1024> dummy(1024);
	TInt blockSize = 0;
	TInt randPos = 0;
	TInt i = 0;

	TInt r = fs.Connect();
	
	fs.SetSessionPath(gSessionPath);
	r = file.Open(fs, gFileB, EFileShareAny|EFileRead);
	
	client.Signal();
	FailIfError(r);
	
	FOREVER
	{
		randPos = Rand(64) * KOneK ; 
		r = file.Seek(ESeekStart, randPos);
		FailIfError(r);
		blockSize = i;
		
		r = file.Read(dummy, blockSize); // Sync operation
		FailIfError(r);
		if(i >= 1023) i = 0;
		else i++;
	}
}


/** Read large blocks in gBaseFile

*/
LOCAL_C TInt ReadLargeBase(TAny* )
	{
	RTest test(_L("test 2"));
	RFs fs;
	RFile file;
	TInt pos = 0;
	TInt blockSize = 128 * KOneK;
	
	TInt r = fs.Connect();
	
	fs.SetSessionPath(gSessionPath);
	r = file.Open(fs,gBaseFile,EFileShareAny|EFileRead);
	
	client.Signal();
	FailIfError(r);
	
	FOREVER
	{	
			r = file.Seek(ESeekStart, pos);
			FailIfError(r);
			
			r = file.Read(gBufReadBPtr, blockSize); // Sync operation
			FailIfError(r);
	}
}

/** Read large blocks in gFileA

*/
LOCAL_C TInt ReadLargeA(TAny* )
{
	RTest test(_L("test 2"));
	RFs fs;
	RFile file;
	TInt blockSize = 128 * KOneK;
	TInt pos = 0;

	TInt r = fs.Connect();

	fs.SetSessionPath(gSessionPath);
	
	r = file.Open(fs,gFileA,EFileShareAny|EFileRead);
	
	client.Signal();
	FailIfError(r);
		
	FOREVER
	{	
		r = file.Seek(ESeekStart, pos);
		FailIfError(r);
		
		r = file.Read(gBufReadBPtr, blockSize); // Sync operation
		FailIfError(r);
	}
}
	
/** Read large blocks in gFileB

*/
LOCAL_C TInt ReadLargeB(TAny* )
{
	RTest test(_L("test 2"));
	RFs fs;
	RFile file;
	TInt blockSize = 128 * KOneK;
	TInt pos = 0;

	TInt r=fs.Connect();
	FailIfError(r);
	
	fs.SetSessionPath(gSessionPath);

	r = file.Open(fs,gFileB,EFileShareAny|EFileRead);

	client.Signal();
	FailIfError(r);
	
	FOREVER
	{
		r = file.Seek(ESeekStart, pos);
		FailIfError(r);
		
		r = file.Read(gBufReadBPtr, blockSize); // Sync operation
		FailIfError(r);
	}
}

/** Small reads from a file
 
	@param xMax 	Maximum position on the x axe
	@param yMax		Maximum position on the y axe
	@param aCase	Type of test. Possible values:
		 - ENoThreads : isolated
		 - ETwoThreadsSame : with two threads accessing same file
		 - ETwoThreadsDif : with two threads accessing dif. files 
*/
LOCAL_C void smallReads(TInt xMax, TInt yMax, TTestState aCase)
	{	
	TInt i = 0;
	TInt j = 0;
	TInt r = 0;
	TInt timeRes = 0;
	
	CreateFile(gBaseFile, Pow(yMax-1));
	
	if(aCase == ETwoThreadsSame) 
		{ // Start two different threads accessing the same file
		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadSmallBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadSmallBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();

		client.Wait();
		client.Wait();
		}
	
	if(aCase == ETwoThreadsDif)
		{ // Start two different threads accessing different files
		CreateFile(gFileA, Pow(yMax-1));
		CreateFile(gFileB, Pow(yMax-1));
		
		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadSmallA, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadSmallB, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();
				
		client.Wait();
		client.Wait();
		}
	
	while(i < xMax)
		{ // Actual accesses and timing to the main file
		j = 0;
		PrintResult(i + 1, j + 1, Pow(i));
		while(j < yMax) 
			{
			if(Pow(i) < KSmallThreshold) 
				{
				timeRes = ReadFromFile(gBaseFile, Rand(Pow(yMax - 1)), Pow(i), Pow(j));
				}
			else
				{ // Too small for only one time measure
				timeRes = ReadFromFileSeveralTimes(gBaseFile, yMax, Pow(i), Pow(j)); 			
				}
			
			gWriting = ETrue; 
			User::After(1000000);

			PrintResultTime(i + 1, j + 2, timeRes); 
			
			gWriting = EFalse;
			
			j++;
			}
		i++;
		}
		
	
	if((aCase == ETwoThreadsSame) || (aCase == ETwoThreadsDif))
		{ // Finish the threads
		DoTestKill();			
		}
	}

/** Large reads from a file
 
 	@param xMax 	Maximum position on the x axe
	@param yMax		Maximum position on the y axe
	@param aCase	Type of test. Possible values:
		 - ENoThreads : isolated
		 - ETwoThreadsSame : with two threads accessing same file
		 - ETwoThreadsDif : with two threads accessing dif. files 
		 - ETwoThreadsDifDif : with two threads accessing dif. files, different block sizes (big/small)
*/
LOCAL_C void largeReads(TInt xMax, TInt yMax, TTestState aCase)
{
	TInt i = 0;
	TInt j = 0;
	TInt r = 0;
	TInt timeRes = 0;
	
	CreateFile(gBaseFile, 10 * KOneK * KOneK); // 10 Mb 
	
	if(aCase == ETwoThreadsSame) 
	{ // Start two different threads accessing the same file

		TRAPD(res, gBufB = HBufC8::NewL(256 * KOneK));
		test(res == KErrNone && gBufB != NULL);
		gBufReadBPtr.Set(gBufSec->Des());

		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadLargeBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadLargeBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();

		client.Wait();
		client.Wait();
	}
	
	if(aCase == ETwoThreadsDif)
	{ // Start two different threads accessing different files
	
		CreateFile(gFileA, KOneK * KOneK);
		CreateFile(gFileB, KOneK * KOneK);
		
		TRAPD(res, gBufB = HBufC8::NewL(256 * KOneK));
		test(res == KErrNone && gBufB != NULL);
		gBufReadBPtr.Set(gBufSec->Des());
		
		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadLargeA, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadLargeB, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();
				
		client.Wait();
		client.Wait();
	}
	
	if(aCase == ETwoThreadsDifDif)
	{ // Start two different threads accessing different files
	
		CreateFile(gFileA, KOneK * KOneK);
		CreateFile(gFileB, KOneK * KOneK);
		
		TRAPD(res,gBufB = HBufC8::NewL(256 * KOneK));
		test(res == KErrNone && gBufB != NULL);
		gBufReadBPtr.Set(gBufSec->Des());
		
		TBuf<20> buf=_L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadLargeA, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadSmallB, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();
				
		client.Wait();
		client.Wait();
	}

	i = 11;
	while(i < xMax )
	{ // Actual accesses and timing to the main file
		j = 0;
		PrintResult(i - 10, j + 1, Pow(i));
		while(j < yMax) 
		{
			TInt size=0;
			if(j == 0) 	size = 100 ; // 100 Kb
			if(j == 1) 	size = KOneK ; // 1 Mb
			if(j == 2) 	size = 10 * KOneK ; // 10 Mb
			
			timeRes = ReadFromFile(gBaseFile, 0, Pow(i), size); 

			gWriting = ETrue; 
			User::After(1000000);

			PrintResultTime(i - 10, j + 2, timeRes); 
				
			gWriting = EFalse;

			j++;
		}
		i++;
	}
	
	if((aCase == ETwoThreadsSame) || (aCase == ETwoThreadsDif) || (aCase == ETwoThreadsDifDif))
	{ // Finish the threads
		DoTestKill();			
		delete gBufB;
	}
	
}

/** Large writes to a file
 
	@param xMax 	Maximum position on the x axe
	@param yMax		Maximum position on the y axe
	@param aCase	Type of test. Possible values:
		 - ENoThreads : isolated
		 - ETwoThreadsSame : with two threads accessing same file
		 - ETwoThreadsDif : with two threads accessing dif. files 
*/
LOCAL_C void largeWrites(TInt xMax, TInt yMax, TTestState aCase)
{
	TInt i = 0;
	TInt j = 0;
	TInt r = 0;
	TInt timeRes = 0;
		
	CreateFile(gBaseFile, 10 * KOneK * KOneK); // 10 Mb 
	
	if(aCase == ETwoThreadsSame) 
	{ // Start two different threads accessing the same file

		TRAPD(res, gBufB = HBufC8::NewL(256 * KOneK));
		test(res == KErrNone && gBufB != NULL);
		gBufReadBPtr.Set(gBufSec->Des());

		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadLargeBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadLargeBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();

		client.Wait();
		client.Wait();
	}
	
	if(aCase == ETwoThreadsDif)
	{ // Start two different threads accessing different files
	
		CreateFile(gFileA, KOneK * KOneK);
		CreateFile(gFileB, KOneK * KOneK);
		
		TRAPD(res, gBufB = HBufC8::NewL(256 * KOneK));
		test(res == KErrNone && gBufB != NULL);
		gBufReadBPtr.Set(gBufSec->Des());
		
		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadLargeA, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadLargeB, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();
				
		client.Wait();
		client.Wait();
	}
	
	i = 11;
	while(i < xMax )
	{ // Actual accesses and timing to the main file
		j = 0;
		PrintResult(i - 10 , j + 1, Pow(i));
		while(j < yMax) 
		{
			TInt size=0;
			if(j == 0) 	size = 100 ; // 100 Kb
			if(j == 1) 	size = KOneK ; // 1 Mb
			if(j == 2) 	size = 10 * KOneK ; // 10 Mb
			timeRes = WriteToFile(gBaseFile, 0, Pow(i), size); 
			
			gWriting = ETrue; 
			User::After(1000000);

			PrintResultTime(i - 10, j + 2, timeRes); 
			
			gWriting = EFalse;			
			j++;
		}
		i++;
	}
	
	if((aCase == ETwoThreadsSame) || (aCase == ETwoThreadsDif) || (aCase == ETwoThreadsDifDif))
	{ // Finish the threads
		DoTestKill();			
		delete gBufB;
	}
	
}

/** Small writes to a file
 
	@param xMax 	Maximum position on the x axe
	@param yMax		Maximum position on the y axe
	@param aCase	Type of test. Possible values:
		 - ENoThreads : isolated
		 - ETwoThreadsSame : with two threads accessing same file
		 - ETwoThreadsDif : with two threads accessing dif. files 
*/
LOCAL_C void smallWrites(TInt xMax, TInt yMax, TTestState aCase)
{
	TInt i = 0;
	TInt j = 0;
	TInt r = 0;
	TInt timeRes = 0;
	
	CreateFile(gBaseFile, Pow(yMax-1));
	
	if(aCase == ETwoThreadsSame) 
	{ // Start two different threads accessing the same file
		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadSmallBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadSmallBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();

		client.Wait();
		client.Wait();
	}
	
	if(aCase == ETwoThreadsDif)
	{ // Start two different threads accessing different files
		CreateFile(gFileA, Pow(yMax-1));
		CreateFile(gFileB, Pow(yMax-1));
		
		TBuf<20> buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		r = gSpeedy.Create(buf, ReadSmallA, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		buf = _L("Speedy");
		buf.AppendNum(ThreadCount++);
		
		r = gSpeedyII.Create(buf, ReadSmallB, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
		FailIfError(r);
		
		gSpeedy.Resume();
		gSpeedyII.Resume();
				
		client.Wait();
		client.Wait();
	}
	
	while(i < xMax)
	{
		j = 0;
		PrintResult(i + 1, j + 1, Pow(i));
		while(j < yMax) 
		{
			if(Pow(i) < KSmallThreshold) 
				{
				timeRes = WriteToFile(gBaseFile, Rand(Pow(yMax-1)), Pow(i), Pow(j)); 
				}
			else
				{
				timeRes = WriteToFileSeveralTimes(gBaseFile, yMax, Pow(i), Pow(j)); 
				}
							
			gWriting = ETrue; 
			User::After(1000000);

			PrintResultTime(i + 1, j + 2, timeRes); 
			
			gWriting = EFalse;
				
			j++;
		}
		i++;
	}
	
	if((aCase == ETwoThreadsSame) || (aCase == ETwoThreadsDif))
	{ // Finish the threads
		DoTestKill();
	}
	
}

/** This test benchmarks small random r/w (e.g. database access)

	@param aSelector Selection array for manual tests
*/
LOCAL_C TInt TestSmall(TAny* aSelector)
{
	
	Validate(aSelector);
	
	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 7;	
	gTestCase = 1;
	gTimeUnit = 1;
	
	PrintHeaders(4, _L("t_fcachebm. Small Random r/w"));
	
	// Small reads
	test.Printf(_L("#~TS_Title_%d,%d: Small reads, no threads \n"), gTestHarness, gTestCase);

	smallReads(KSmallRow, KSmallCol, ENoThreads); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Small reads, threads accessing same file \n"), 
														gTestHarness, gTestCase);
	
	smallReads(KSmallRow, KSmallCol, ETwoThreadsSame); 
	
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Small reads, threads accessing dif files \n"),
															 gTestHarness, gTestCase);
	
	smallReads(KSmallRow, KSmallCol, ETwoThreadsDif); 
	
	gTestCase++;
	DeleteAll(gSessionPath);
	
	// Small writes test case 
	
	test.Printf(_L("#~TS_Title_%d,%d: Test small writes\n"), gTestHarness, gTestCase);

	smallWrites(KSmallRow, KSmallCol, ENoThreads); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Small writes, threads accessing same file \n"), 
															gTestHarness, gTestCase);
	smallWrites(KSmallRow, KSmallCol, ETwoThreadsSame); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Small writes, threads accessing dif files \n"),
															 gTestHarness, gTestCase);
	smallWrites(KSmallRow, KSmallCol, ETwoThreadsDif); 

	gTestCase++;
	DeleteAll(gSessionPath);
	test.Printf(_L("#~TestEnd_%d\n"), gTestHarness);
	
	return(KErrNone);
}

/** This test benchmarks large sequential r/w (e.g. MM)

	@param aSelector Selection array for manual tests
*/
LOCAL_C TInt TestLarge(TAny* aSelector)
{
	
	Validate(aSelector);

	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 8;	
	gTestCase = 1;
	gTimeUnit = 1000;
	
	PrintHeaders(3, _L("t_fcachebm. Large sequential r/w"));
	
	// Large reads
	test.Printf(_L("#~TS_Title_%d,%d: Large sequential reads\n"), gTestHarness, gTestCase);
	
	largeReads(KLargeRow, KLargeCol, ENoThreads); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Large sequential reads, threads accessing same file\n"), 
																	gTestHarness, gTestCase);
	largeReads(KLargeRow, KLargeCol, ETwoThreadsSame); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Large sequential reads, threads accessing dif files\n"), 
																	gTestHarness, gTestCase);
	largeReads(KLargeRow, KLargeCol, ETwoThreadsDif); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Large sequential reads, threads accessing dif files some big some small blocks\n"),
																	gTestHarness, gTestCase);
	largeReads(KLargeRow, KLargeCol, ETwoThreadsDifDif); 

	gTestCase++;
	DeleteAll(gSessionPath);

	// Large writings		
	test.Printf(_L("#~TS_Title_%d,%d: Large sequential writes\n"), gTestHarness, gTestCase);

	largeWrites(KLargeRow, KLargeCol, ENoThreads); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Large sequential writes, threads accessing same file\n"), 
																	gTestHarness, gTestCase);
	largeWrites(KLargeRow, KLargeCol, ETwoThreadsSame); 

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Large sequential writes, threads accessing dif files\n"), 
																	gTestHarness, gTestCase);
	largeWrites(KLargeRow, KLargeCol, ETwoThreadsDif); 

	gTestCase++;
	DeleteAll(gSessionPath);
	test.Printf(_L("#~TestEnd_%d\n"), gTestHarness);
	
	return(KErrNone);
}

/** Writes aSize bytes of data in aBlockSize during aTime seconds 
	if the aSize bps is not met, it fails

	@param f 			File to write to
	@param aSize 		Size in bytes of data to be written 
	@param aBlockSize 	Block size to be used
	@param aTime 		Time during which the write has to happen in seconds 
*/
LOCAL_C TBool writeStr( RFile f, TInt aSize, TInt aBlockSize, TInt aTime) 
{
	TInt r = 0, j = 0;
	TTime startTime, endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TTimeIntervalMicroSeconds32 timeLeft(0);
	TBool onTarget = ETrue;
	
	TInt time;

	TInt i = 0;
	while((i < aTime) && onTarget) 
	{
		// If measuring the CPU time
		
		startTime.HomeTime();	
		j = 0;
		while(j < aSize)
			{
				r = f.Write(gBufWritePtr, aBlockSize);
				FailIfError(r);
				j += aBlockSize;
			}					
		endTime.HomeTime();
		
		timeTaken = endTime.MicroSecondsFrom(startTime);	
		time = I64LOW(timeTaken.Int64());
		if(time > KOneSec) 
		{
			onTarget = EFalse; 			
		}
		
		timeLeft = KOneSec - time;
		if(timeLeft.Int() >= 0) 
		{
			User::After(timeLeft);
		}
		i++;
	}
	
	return onTarget;
}

/** Reads streaming

*/
LOCAL_C TInt ReadStream(TAny*)
{
	RTest test(_L("test 2"));
	RFs fs;
	RFile file;
	TInt j = 0;
	TTime startTime, endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TTimeIntervalMicroSeconds32 timeLeft(0);
	TInt time;
	TInt size , currentPos = 0; 

	TInt r = fs.Connect();
	
	fs.SetSessionPath(gSessionPath);
	
	r = file.Open(fs, gStreamFile, EFileShareAny|EFileRead);
	
	client.Signal();

	FailIfError(r);
	r = file.Size(size);
	FailIfError(r);

	FOREVER
	{
		startTime.HomeTime();	
		j = 0;
		while(j < (gCurrentSpeed * KOneK))
			{
				r=file.Read(gBufReadPtr,gCurrentBS);
				FailIfError(r);
				j += gCurrentBS;
			}					
		endTime.HomeTime();
		
		timeTaken = endTime.MicroSecondsFrom(startTime);	
		time = I64LOW(timeTaken.Int64());
		
		if(time > KOneSec) 
		{
			test.Printf(_L("Background Thread: Speed failed to be achieved: %d kbps\n"), gCurrentSpeed);			
		}
		

		
		timeLeft = KOneSec - time;
		User::After(timeLeft);
		currentPos += (gCurrentSpeed * KOneK); 
		r = file.Size(size);
		FailIfError(r);
		if(currentPos > size ) 
		{
			currentPos = 0;
			file.Seek(ESeekStart, currentPos);
		}

	}
	
}

/** Test case layout, read/write at aSpeed during aWtime and aRTime

	@param aSpeed 		Target speed in kbps
	@param aBlockSize 	Block size for the I/O operation
	@param aWTime		Writing time
	@param aRTime 		Reading time 		
*/
LOCAL_C void streamIt ( TInt aSpeed, TInt aBlockSize, TInt aWTime, TInt aRTime, TInt aStep)
{
	TInt iSize = (aSpeed * KTobps) / KByte;  // Size in bytes
	RFile file;
	TInt pos = 0;
	TInt r = 0;
	
	PrintResult(aStep, 1, aBlockSize);
	
	r = file.Replace(TheFs, gStreamFile, EFileShareAny|EFileWrite);
	FailIfError(r);

	// Streaming to the media during aWTime seconds
	if(writeStr(file, iSize, aBlockSize, aWTime))
	{	//Pass (1)
		PrintResult(aStep, 2, 1); 
	}
	else 
	{ //Fail (0)
		PrintResult(aStep, 2, 0); 
	}
	
	// Create a different thread for reading from the beginning during aRTime
	TBuf<20> buf = _L("Speedy");
	buf.AppendNum(ThreadCount++);
	
	gCurrentSpeed = aSpeed; 
	gCurrentBS = aBlockSize; 
	r = gSpeedy.Create(buf, ReadStream, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

	gSpeedy.Resume();
	client.Wait();

	// Keep writing during the time the other thread is reading
	if(writeStr(file, iSize, aBlockSize, aRTime))
	{ //Pass (1)
		PrintResult(aStep, 3, 1); 	
	}
	else
	{ //Fail (0)
		PrintResult(aStep, 3, 0); 
	}
	
	// Writing from the beginning again 
	file.Seek(ESeekStart, pos);
	if(writeStr(file, iSize, aBlockSize, aRTime))
	{ //Pass (1)
		PrintResult(aStep, 4, 1); 	
	}
	else
	{ //Fail (0)
		PrintResult(aStep, 4, 0); 
	}
	

	// Kill the thread for reading
	gSpeedy.Kill(KErrNone);
	FailIfError(r);
	gSpeedy.Close();	
	
	file.Close();	
}

/** Iterating through different blocksizes

	@param aSpeed Speed at which the test happens
*/
LOCAL_C void streaming(TInt aSpeed)
{
	TInt i = 9; // Pow(i) = 512 bytes
	TInt blockSize = 0;
	TInt testStep = 1;
	
	while( i < 15 ) // Pow(i) = 16 Kb
	{
		blockSize = Pow(i) ; 
		streamIt(aSpeed, blockSize, 5 * 60, 15, testStep++); // 5 minutes writing , then 15 secs reading
		i++;
	}
}

/** High level test routine. Different test cases executed

	@param aSelector 	Test case configuration in case of manual execution
*/
LOCAL_C TInt TestStreaming(TAny* aSelector)
{
	
	Validate(aSelector);
	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 9;	
	gTestCase = 1;


	PrintHeaders(5, _L("t_fcachebm. Streaming"));
	
	test.Printf(_L("#~TS_Title_%d,%d: Writing / reading at 100 kbps\n"), 
												gTestHarness, gTestCase);
	
	streaming(100);
	
	test.Printf(_L("#~TS_Title_%d,%d: Writing / reading at 200 kbps\n"), 
												gTestHarness, ++gTestCase);

	streaming(200);

	test.Printf(_L("#~TS_Title_%d,%d: Writing / reading at 500 kbps\n"), 
												gTestHarness, ++gTestCase);

	streaming(500);


	DeleteAll(gSessionPath);

	// Start the small random reads in the background
	CreateFile(gBaseFile, Pow(KSmallCol-1));
	
	TBuf<20> buf=_L("Speedy");
	buf.AppendNum(ThreadCount++);
	
	TInt r = gSpeedyII.Create(buf, ReadSmallBase, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

	gSpeedyII.Resume();
	client.Wait();
	
	// Measure the throughput with background activity
	test.Printf(_L("#~TS_Title_%d,%d: Writing / reading at 100 kbps, while small reads\n"), 
																gTestHarness, ++gTestCase);

	streaming(100);	

	test.Printf(_L("#~TS_Title_%d,%d: Writing / reading at 200 kbps, while small reads\n"), 
																gTestHarness, ++gTestCase);

	streaming(200);
	
	test.Printf(_L("#~TS_Title_%d,%d: Writing / reading at 500 kbps, while small reads\n"), 
																gTestHarness, ++gTestCase);
	
	streaming(500);
	
	// Kill the small random reads and writes 
	gSpeedyII.Kill(KErrNone);
	FailIfError(r);
	
	gSpeedyII.Close();	
	
	DeleteAll(gSessionPath);

	gTestCase++;
	test.Printf(_L("#~TestEnd_%d\n"), gTestHarness);
	
	return(KErrNone);
}

/** It goes automatically through all the options

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestAll(TAny* aSelector)
{
 	TestSmall(aSelector);
 	TestLarge(aSelector);
 	TestStreaming(aSelector);

	return(KErrNone);
}

/** Call all tests

*/
GLDEF_C void CallTestsL()
{
	TBuf16<25> temp;
	
	TInt r=client.CreateLocal(0);
	FailIfError(r);
	
	// Setting up the environment and creating the needed files
	gSessionPath = _L("?:\\F32-TST\\");
	gSessionPath[0] = (TText) gDriveToTest;
	
	FileNamesGeneration(temp, 8, 0, 1);	
	gBaseFile = gSessionPath;
	gBaseFile.Append(temp);
	
	FileNamesGeneration(temp, 8, 1, 1);	
	gFileA = gSessionPath;
	gFileA.Append(temp);
	
	FileNamesGeneration(temp, 8, 2, 1);	
	gFileB = gSessionPath;
	gFileB.Append(temp);

	FileNamesGeneration(temp, 8, 3, 1);	
	gStreamFile = gSessionPath;
	gStreamFile.Append(temp);

	TRAPD(res,gBuf = HBufC8::NewL(256 * KOneK));
	test(res == KErrNone && gBuf != NULL);
		
	gBufWritePtr.Set(gBuf->Des());
	FillBuffer(gBufWritePtr, 256 * KOneK, 'A');
	
	TRAPD(res2, gBufSec = HBufC8::NewL(256 * KOneK));
	test(res2 == KErrNone && gBufSec != NULL);
	gBufReadPtr.Set(gBufSec->Des());

	TVolumeInfo volInfo;
	TInt drive;
	
	r = TheFs.CharToDrive(gDriveToTest,drive);
	FailIfError(r);
	r = TheFs.Volume(volInfo, drive);
	FailIfError(r);
	
	FormatFat(gSessionPath[0]-'A');
	TheFs.MkDirAll(gSessionPath);

	RThread noisy; 
	TBuf<20> buf = _L("Noisy");
	r = noisy.Create(buf, noise, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);
	
	noisy.Resume();

	CSelectionBox* TheSelector=CSelectionBox::NewL(test.Console());
	
	if(gMode == 0) 
	{ // Manual
		gSessionPath=_L("?:\\");
		TCallBack smallOps(TestSmall, TheSelector);
		TCallBack largeOps(TestLarge, TheSelector);
		TCallBack simulOps(TestStreaming, TheSelector);
		TCallBack tAll(TestAll, TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Small random r/w"), smallOps);
		TheSelector->AddLineL(_L("Large conseq r/w"), largeOps);
		TheSelector->AddLineL(_L("Streaming"), simulOps);
		TheSelector->AddLineL(_L("Execute all options"), tAll);
		TheSelector->Run();
	}
	else 
	{ // Automatic
		TestAll(TheSelector);
	}
	
	DeleteAll(gSessionPath);
			
	client.Close();
	delete TheSelector;
	delete gBuf;
	delete gBufSec;
	noisy.Kill(KErrNone);
	noisy.Close();	
}
