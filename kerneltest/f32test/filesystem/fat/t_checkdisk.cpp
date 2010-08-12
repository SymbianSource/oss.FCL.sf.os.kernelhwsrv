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
// f32test\server\t_CHECKDISK.cpp
//
//
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
//#include <e32svr.h>
//#include <f32dbg.h>
#include "t_server.h"
//#include <e32twin.h>

RTest test(_L("T_CHECKDISK"));

RFs TheFs;
TInt gDrive;
TFileName gSessionPath;
TChar gDriveToTest;


HBufC8* gBuf = NULL;
TPtr8 gBufReadPtr(NULL, 0);
HBufC8* gBufSec = NULL;
TPtr8 gBufWritePtr(NULL, 0);

const TInt KOneK = 1024;
const TInt KOneMeg = KOneK * 1024;
const TInt KBlockSize = KOneK * 129 ;
const TInt KWaitRequestsTableSize = 70;

TInt gBigFileSize = 0;
TInt gSmallFileSize = 0;
TInt64 gMediaSize = 0;
TBool gSkip=EFalse;
TInt writeSize = KBlockSize;
TInt seekSize = 0;
TSeek seekType = ESeekAddress;
TInt reduceSize = 0;

TTimeIntervalMicroSeconds32 gTimeTakenBigFile(0);
TBuf16<45> gSmallFile, gBigFile;
LOCAL_D TInt gNextFile=0;
TTime gTime1;
TTime gTime2;

LOCAL_D RSemaphore gSync;

// Concurrent Threads
RThread gBig;
RThread gSmall;
LOCAL_D RSemaphore client;
LOCAL_D const TInt KHeapSize=0x4000;
LOCAL_D const TInt KMaxHeapSize=0x100000;
TRequestStatus gStatus[KWaitRequestsTableSize];

enum TTestState
	{
	EThreadWait,
	EThreadSignal,
	ENoThreads
	};

//
// Generates a name of the form FFFFF*<aPos>.TXT (aLong.3)
//
GLDEF_C void FileNameGen(TDes16& aBuffer, TInt aLong, TInt aPos)
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




//
// Expects a drive letter as a parameter
//
LOCAL_C void parseCommandLine()
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TInt r=0;

	if(token.Length()!=0)
		{
		gDriveToTest=token[0];
		gDriveToTest.UpperCase();
		}
	else
		{
		gDriveToTest='C';
		}

		r=TheFs.CharToDrive(gDriveToTest,gDrive);
		test_KErrNone(r);
		gSessionPath=_L("?:\\F32-TST\\");
		gSessionPath[0]=(TText)gDriveToTest;
		test.Printf(_L("\nCLP=%S\n"),&token);
	}

//
// Fills a buffer with character aC
//
LOCAL_C void FillBuffer(TDes8& aBuffer, TInt aLength, TChar aC)
	{
	test (aBuffer.MaxLength() >= aLength);
	for(TInt i=0; i<aLength; i++)
		{
		aBuffer.Append(aC);
		}
	}

//
// Waits for all the TRequestStatus in status[] to complete
//
LOCAL_C void WaitForAll(TRequestStatus status[], TInt aSize)
	{
	TInt i=0;

	while(i<aSize)
		{
		User::WaitForRequest(status[i++]);
		}
	}


//
// Writes a file synchronously in blocks of aBlockSize size
// this function can be called from another thread, therefore requires its own RTest instance
LOCAL_C TInt WriteFile(RFs& fs, TDes16& aFile, TInt aSize, TInt aBlockSize, TTestState aState)
	{
	RTest test(_L(""));

	TInt r=0;
	RFile fileWrite;

	test(aBlockSize>0);				// Block size must be greater than 0

	if(aState==EThreadWait)
		{
		gSync.Wait();
		}
	r=fileWrite.Replace(fs,aFile,EFileShareAny|EFileWrite);
	test_KErrNone(r);

	TInt j=0;
	while(j<aSize)
		{
		r=fileWrite.Write(gBufWritePtr, writeSize);
		test_KErrNone(r);
		if(seekType)
			{
			r=fileWrite.Seek(seekType, seekSize);
			test_KErrNone(r);

			if (writeSize + reduceSize >= 0)
				writeSize += reduceSize;

			writeSize = Min(writeSize, gBufWritePtr.Length());

			}
		if((j==0)&&(aState==EThreadSignal))
			{
			gSync.Signal();
			}
		j+=aBlockSize;
		}

	fileWrite.Close();

	return KErrNone;
	}

LOCAL_C void IniStatus(TRequestStatus aStatus[], TInt aSize)
	{
	TInt i=0;

	while(i<aSize)
		{
		aStatus[i++]=KRequestPending;
		}
	}

//
// Write big file
// This is a thread function, therefore requires its own RTest instance
LOCAL_C TInt WriteBigFile(TAny* )
	{
	RTest test(_L(""));
	RFs fs;
	TInt r=fs.Connect(gDriveToTest);
	test_KErrNone(r);

	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	WriteFile(fs, gBigFile, gBigFileSize, KBlockSize, EThreadSignal);
	gTime1.HomeTime();

	client.Signal();
	return ETrue;
	}

//
// Writes a file asynchronously in blocks of aBlockSize size
// this function can be called from another thread, therefore requires its own RTest instance
LOCAL_C void WriteFileAsync(RFs& fs, RFile& aFileWrite, TDes16& aFile, TInt aSize, TInt aBlockSize, TRequestStatus aStatus[])
	{
	RTest test(_L(""));

	TInt r=0;

	test(aBlockSize>0);				// Block size must be greater than 0
	test((aSize%aBlockSize)==0); 	// Ensure the size of the file is a multiple of the block size

	r=aFileWrite.Replace(fs,aFile,EFileShareAny|EFileWrite);
	test_KErrNone(r);

	TInt j=0,i=0;
	while(j<aSize)
		{
		aFileWrite.Write(gBufWritePtr,aStatus[i++]);
		j+=aBlockSize;
		}
	}



//
// Write big file async
// This is a thread function, therefore requires its own RTest instance
LOCAL_C TInt WriteBigFileAsync(TAny* )
	{
	RTest test(_L(""));
	RFs fs;
	TInt r=fs.Connect(gDriveToTest);
	test_KErrNone(r);

	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);


	RFile bigFile;

	IniStatus(gStatus,KWaitRequestsTableSize);
	WriteFileAsync(fs, bigFile, gSmallFile, gBigFileSize, KBlockSize,gStatus);
	gSync.Signal();
	WaitForAll(gStatus, gBigFileSize/KBlockSize);

	return ETrue;
	}

static void TestClientDies()
	{
	test.Next(_L("Client dying unexpectedly"));

	TInt r=0;
	TBuf<20> unit=_L("?:\\");
	unit[0]=(TText)gDriveToTest;

	//-------------------------------------------
	test.Printf(_L("Sync test\n"));

	r=gBig.Create(_L("TEST1"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Async test\n"));
	r=gSmall.Create(_L("TEST2"),WriteBigFileAsync,KDefaultStackSize*2,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);

	gSmall.Resume();
	gSync.Wait();

	gSmall.Kill(-2);
	gSmall.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Testing for size not multiple of blocksize\n"));

	writeSize = 5000;
	r=gBig.Create(_L("TEST3"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);
	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Testing with seek current and write inside the file boundary\n"));

	writeSize = 5000;
	seekType = ESeekCurrent;
	seekSize = -5000;
	reduceSize = -3000;
	r=gBig.Create(_L("TEST4"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);
	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Testing with seek current and overwrite entire file\n"));
	writeSize = 5000;
	seekType = ESeekCurrent;
	seekSize = -5000;
	reduceSize = 0;
	r=gBig.Create(_L("TEST5"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);
	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Testing with seek current and write outside the file boundary\n"));

	writeSize = 5000;
	seekType = ESeekCurrent;
	seekSize = -5000;
	reduceSize = 5000;
	r=gBig.Create(_L("TEST6"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);
	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Testing with seek current and write within the file boundary\n"));
	writeSize = 5000;
	seekType = ESeekCurrent;
	seekSize = -3000;
	reduceSize = -4000;
	r=gBig.Create(_L("TEST7"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);
	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Testing with seek current and write exactly to file size\n"));
	writeSize = 5000;
	seekType = ESeekCurrent;
	seekSize = -3000;
	reduceSize = -3000;
	r=gBig.Create(_L("TEST8"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);
	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	//-------------------------------------------
	test.Printf(_L("Testing with seek current and write greater than file size\n"));
	writeSize = 5000;
	seekType = ESeekCurrent;
	seekSize = -3000;
	reduceSize = 10000;
	r=gBig.Create(_L("TEST9"),WriteBigFile,KDefaultStackSize,KHeapSize,KMaxHeapSize,NULL);
	test_KErrNone(r);
	gBig.Resume();
	gSync.Wait();

	gBig.Kill(-2);
	gBig.Close();
	User::After(500000);

	r=TheFs.CheckDisk(unit);
	test_Value(r, r == KErrNone || r == KErrNotSupported);

	TInt retries = 0;

	do
		{
		r=TheFs.ScanDrive(unit);
		if (r != KErrNone)
			test.Printf(_L("ScanDrive() returned %d\n"), r);
		if (r == KErrInUse)
			User::After(500000);
		}
	while (r == KErrInUse && ++retries < 5);

	test_Value(r, r == KErrNone || r == KErrNotSupported);

	}


//---------------------------------------------------------------------
/**
    Test that CheckDisk will not cause stack overflow on very deep directory structure.
*/
void TestCheckDisk_VeryDeepDirectory()
	{
	test.Next(_L("Testing deep dir structure check"));

	TInt nRes;
	TBuf<20> unit=_L("?:\\");
	unit[0]=(TText)gDriveToTest;

	//-- 1. create deep dir structure, like \\0\\1\\2\\...... 90 levels deep
	const TInt KMaxDirDepth = 90;

	test.Printf(_L("Creating directory with %d subdirs.\n"),KMaxDirDepth);

	TFileName fn;
	for(TInt i=0; i<KMaxDirDepth; ++i)
		{
		fn.AppendFormat(_L("\\%d"), i%10);
		}
	fn.Append(_L("\\"));

	nRes = TheFs.MkDirAll(fn);
	test_Value(nRes, nRes == KErrNone || nRes == KErrAlreadyExists);

	//-- 2. invoke Check Disk and ensure that target doesn't die from stack overflow.
	test.Printf(_L("Running Check Disk...\n"));
	nRes = TheFs.CheckDisk(unit);

	test_Value(nRes, nRes == KErrNone || nRes == KErrTooBig);
	}


//---------------------------------------------------------------------
//
// This test tries to read a small file while writing a big one
//
GLDEF_C void CallTestsL()
	{
	TBuf16<45> dir;

	test.Next(_L("Preparing the environmnet\n"));

	FileNameGen(gSmallFile, 8, gNextFile++);
	FileNameGen(gBigFile, 8, gNextFile++);
	dir=gSessionPath;
	dir.Append(gSmallFile);
	gSmallFile=dir;
	dir=gSessionPath;
	dir.Append(gBigFile);
	gBigFile=dir;

	TRAPD(res,gBuf = HBufC8::NewL(KBlockSize+1));
	test(res == KErrNone && gBuf != NULL);

	gBufWritePtr.Set(gBuf->Des());
	FillBuffer(gBufWritePtr, KBlockSize, 'B');

	TRAPD(res2,gBufSec = HBufC8::NewL(KBlockSize+1));
	test(res2 == KErrNone && gBufSec != NULL);
	gBufReadPtr.Set(gBufSec->Des());

	//---------------------------

	TestClientDies();
	TestCheckDisk_VeryDeepDirectory();

	delete gBuf;
	delete gBufSec;
	}

LOCAL_C void DoTests()
	{
 	TInt r=0;

 	r=client.CreateLocal(0);
	test_KErrNone(r);

  	r=gSync.CreateLocal(0);
	test_KErrNone(r);

	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);

	r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);

	TheFs.ResourceCountMarkStart();
	TRAP(r,CallTestsL());
	test_KErrNone(r);
	TheFs.ResourceCountMarkEnd();
	}

TBool CheckForDiskSize()
	{
	TVolumeInfo volInfo;
	TInt r = TheFs.Volume(volInfo, gDrive);
	test_KErrNone(r);

	gMediaSize = volInfo.iFree;
	gSmallFileSize = KBlockSize;
	gBigFileSize = KBlockSize*20;
	while(((2*gBigFileSize)+KOneMeg) > gMediaSize )
		{
			gBigFileSize -= (2*KBlockSize);
		}

	if(gBigFileSize< (3*gSmallFileSize))
		return EFalse;
	else
		return ETrue;
	}

void Format(TInt aDrive)
	{

	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,driveBuf,EQuickFormat,count);
	test_KErrNone(r);

	while(count)
		{
		TInt r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}


static TBool IsFAT(RFs &aFsSession, TInt aDrive)
{
	_LIT(KFatName, "Fat");

	TFileName f;
	TInt r = aFsSession.FileSystemName(f, aDrive);
	test_Value(r, r == KErrNone || r == KErrNotFound);
	return (f.CompareF(KFatName) == 0);
}


GLDEF_C TInt E32Main()
	{

	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	__UHEAP_MARK;
	test.Title();
	test.Start(_L("Starting tests..."));
	parseCommandLine();

	TInt r = TheFs.Connect();
	test_KErrNone(r);

	TDriveInfo info;
	TVolumeInfo volInfo;
	r=TheFs.Drive(info,gDrive);
	test_KErrNone(r);

	if(info.iMediaAtt&KMediaAttVariableSize)
		{// Not testing in RAM Drives
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

	if(!IsFAT(TheFs, gDrive))
        {
		test.Printf(_L("Tests skipped on non-FAT drive\n"));
		goto out;
        }

	if ((volInfo.iDrive.iMediaAtt & KMediaAttFormattable))
		Format(gDrive);

	if(CheckForDiskSize())
		{
		DoTests();
		if ((volInfo.iDrive.iMediaAtt & KMediaAttFormattable))
		   Format(gDrive);
		}
	else
		{
		test.Printf(_L("Skipping tests due to lack of space to perform them in this unit\n"));
		}
out:
	test.End();

	TheFs.Close();
	test.Close();

	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }
