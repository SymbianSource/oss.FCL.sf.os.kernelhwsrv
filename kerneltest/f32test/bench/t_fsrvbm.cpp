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
// f32test\bench\t_fsrvbm.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_select.h"
#include "../server/t_server.h"

GLDEF_D RTest test(_L("File Server Benchmarks"));

LOCAL_D RSemaphore client;
LOCAL_D TInt speedCount;
LOCAL_D const TInt KHeapSize=0x2000;
LOCAL_D TBuf8<4096> buf;
//
LOCAL_D TDriveList gDriveList;
//
LOCAL_D TInt gLocalDrive;
LOCAL_D TInt gLocalDriveReadSize;
LOCAL_D TInt gLocalDriveWriteSize;
//
LOCAL_D TFileName gFindEntryDir;
LOCAL_D TInt gFindEntrySearchStart;
LOCAL_D TInt gFindEntrySearchFinish;
//
LOCAL_D TInt gSeekPos1;
LOCAL_D TInt gSeekPos2;
LOCAL_D TFileName gSeekFile;

LOCAL_D TInt ThreadCount=0;

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

const TInt64 KGb  	= 1 << 30;
const TInt64 K2GB 	= 2 * KGb;
const TInt64 K3GB   = 3 * KGb;

LOCAL_D TInt64 gLSeekPos1;
LOCAL_D TInt64 gLSeekPos2;

enum TSelectedTest
    {
	ELocalDriveTest, EFindEntryTest, EFileSeekTest, EFileSeekRFile64Test
	};
#else 
enum TSelectedTest
    {
	ELocalDriveTest, EFindEntryTest, EFileSeekTest
	};
#endif  ////SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API


// RFs::CheckDisk() will return an error if the directory depth exceeds this value :
const TInt KCheckDskMaxRecursionLevel = 50;

LOCAL_C void FormatFat(TDriveUnit aDrive)
//
// Call all RFormat methods
//
	{

	RFormat format;
	TPckgBuf<TInt> count;
	TInt r=format.Open(TheFs,aDrive.Name(),EHighDensity,count());
	test(r==KErrNone);
	test(count()==100);
	TRequestStatus status;
	while (count())
		{
		format.Next(count,status);
		User::WaitForRequest(status);
		test(status==KErrNone);
		}
	format.Close();
	}

LOCAL_C void DoTest(TThreadFunction aFunction)
//
// Do a speed test
//
	{

	RThread speedy;
	TBuf<8> buf=_L("Speedy");
	buf.AppendNum(ThreadCount++);
	TInt r=speedy.Create(buf,aFunction,KDefaultStackSize,KHeapSize,KHeapSize,NULL);
	test(r==KErrNone);
//
    speedy.SetPriority(EPriorityLess);
	speedy.Resume();
	client.Wait();

	r = TheFs.CheckDisk(gSessionPath);
	test (r == KErrNone);

//
    User::After(300000);
    TInt b=speedCount;
    User::After(3000000);
    TInt n=speedCount;
    test.Printf(_L(" Completed %d calls per second\n"),(n-b)/3);

	test(TheFs.CheckDisk(gSessionPath) == KErrNone);

//
	speedy.Kill(KErrNone);
	test(r==KErrNone);
	speedy.Close();
	}

LOCAL_C TInt rawReadData(TAny*)
//
// The entry point for the speed test thread.
//
	{

	speedCount=0;
	TBusLocalDrive localDrive;
    TBool changed;
	localDrive.Connect(gLocalDrive,changed);
	client.Signal();
	FOREVER
		{
		localDrive.Read(512+32,gLocalDriveReadSize,buf);
		speedCount++;
		}
//	return(KErrNone);
	}

LOCAL_C TInt rawWriteData(TAny*)
//
// The entry point for the speed test thread.
//
	{

	speedCount=0;
	TBusLocalDrive localDrive;
    TBool changed;
	localDrive.Connect(gLocalDrive,changed);
	buf.SetLength(gLocalDriveWriteSize);
	client.Signal();
	FOREVER
		{
		localDrive.Write(512+32,buf);
		speedCount++;
		}
//	return(KErrNone);
	}

LOCAL_C void TestLocalDriveRead(TInt drive,TInt readsize)
//
// Test TBusLocalDrive read
//
	{

	test.Printf(_L("TBusLocalDrive %d Reading %d bytes"),drive,readsize);
#if defined(__WINS__)
	if (drive==EDriveX)
		drive=1;
	else if (drive==EDriveY)
		drive=0;
#endif
	gLocalDrive=drive;
	gLocalDriveReadSize=readsize;
	DoTest(rawReadData);
	}

LOCAL_C void TestLocalDriveWrite(TInt drive,TInt writesize)
//
// Test TBusLocalDrive write
//
	{

	test.Printf(_L("TBusLocalDrive %d Writing %d bytes"),drive,writesize);
#if defined(__WINS__)
	if (drive==EDriveX)
		drive=1;
	else if (drive==EDriveY)
		drive=0;
#endif
	gLocalDrive=drive;
	gLocalDriveWriteSize=writesize;
	DoTest(rawWriteData);
	}

LOCAL_C TInt FindEntryBounce(TAny*)
//
// Find entries in hierarchy
//
	{

	speedCount=0;
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);
	r=fs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	client.Signal();
	FOREVER
		{
		TEntry entry;
		r=fs.Entry(gFindEntryDir,entry);
		test(r==KErrNone);
		r=fs.Entry(_L("\\F32-TST"),entry);
		test(r==KErrNone);
		speedCount++;
		}
//	fs.Close();
//	return(KErrNone);
	}

LOCAL_C void TestFindEntryBounce(TInt aDepth)
//
// Find entries at different depths
//
	{

	test.Printf(_L("Find entry 1 then find entry %d"),aDepth);
	gFindEntryDir=_L("\\F32-TST\\");
	TInt i;
	for(i=0;i<aDepth;i++)
		{
		gFindEntryDir+=_L("X");
		gFindEntryDir.AppendNum(i);
		gFindEntryDir+=_L("\\");
		}
	gFindEntryDir+=_L("X");
	gFindEntryDir.AppendNum(i);
	DoTest(FindEntryBounce);
	}

LOCAL_C TInt FindEntrySearch(TAny*)
//
// Find entries in hierarchy
//
	{

	speedCount=0;
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);
	r=fs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	client.Signal();
	FOREVER
		{
		TFileName temp=gFindEntryDir;
		for(TInt i=gFindEntrySearchStart;i<gFindEntrySearchFinish;i++)
			{
			temp+=_L("X");
			temp.AppendNum(i);
			TEntry entry;
			r=fs.Entry(temp,entry);
			test(r==KErrNone);
			temp+=_L("\\");
			}
		speedCount++;
		}
//	fs.Close();
//	return(KErrNone);
	}

LOCAL_C void TestFindEntrySearch(TInt aStart, TInt aFinish)
//
// Find entries at different depths
//
	{

	test.Printf(_L("Find entries %d to %d"),aStart,aFinish);
	gFindEntrySearchStart=aStart;
	gFindEntrySearchFinish=aFinish;
	gFindEntryDir=_L("\\F32-TST\\");
	for(TInt i=0;i<aStart;i++)
		{
		gFindEntryDir+=_L("X");
		gFindEntryDir.AppendNum(i);
		gFindEntryDir+=_L("\\");
		}
	DoTest(FindEntrySearch);
	}

LOCAL_C TInt FileSeekTest(TAny*)
//
// Read 16bytes at different locations
//
	{

	speedCount=0;
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);
	r=fs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	RFile f;
	r=f.Open(fs,gSeekFile,EFileRead); // 3rd arg was EFileRandomAccess, but this no longer exists
	test(r==KErrNone);
	client.Signal();
	FOREVER
		{
		r=f.Read(gSeekPos1,buf,16);
		test(r==KErrNone);
		r=f.Read(gSeekPos2,buf,16);
		test(r==KErrNone);
		speedCount++;
		}
//	f.Close();
//	fs.Close();
//	return(KErrNone);
	}

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

LOCAL_C TInt FileSeekRFile64Test(TAny*)
//
// Read 16bytes at different locations
//
	{

	speedCount=0;
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);
	r=fs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	RFile64 f;
	r=f.Open(fs,gSeekFile,EFileRead); // 3rd arg was EFileRandomAccess, but this no longer exists
	test(r==KErrNone);
	client.Signal();
	FOREVER
		{
		r=f.Read(gLSeekPos1,buf,16);
		test(r==KErrNone);
		r=f.Read(gLSeekPos2,buf,16);
		test(r==KErrNone);
		speedCount++;
		}
	}

LOCAL_C void TestSeek64(TInt64 aLoc1, TInt64 aLoc2)
//
// Read 16bytes at different locations
//
	{

	test.Printf(_L("Read 16bytes at positions %ld and %ld"),aLoc1,aLoc2);
	
	gLSeekPos1=aLoc1;
	gLSeekPos2=aLoc2;
	DoTest(FileSeekRFile64Test);
	}

#endif //SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

LOCAL_C void TestSeek(TInt aLoc1, TInt aLoc2)
//
// Read 16bytes at different locations
//
	{

	test.Printf(_L("Read 16bytes at positions %d and %d"),aLoc1,aLoc2);
	gSeekPos1=aLoc1;
	gSeekPos2=aLoc2;
	DoTest(FileSeekTest);
	}

LOCAL_C void InitializeDrive(CSelectionBox* aSelector)
//
// Get the drive into a good state
//
	{

	TDriveUnit drive=((CSelectionBox*)aSelector)->CurrentDrive();
	gSessionPath[0]=TUint8('A'+drive);
	TInt r=TheFs.SetSessionPath(gSessionPath);
	test(r==KErrNone);
	TDriveInfo driveInfo;
	r=TheFs.Drive(driveInfo);
	test(r==KErrNone);
	if (driveInfo.iType==EMediaNotPresent)
		{
		test.Printf(_L("ERROR: MEDIA NOT PRESENT <Press return to continue>\n"));
		test.Getch();
		return;
		}
	r=TheFs.MkDirAll(gSessionPath);
	test(r==KErrCorrupt || r==KErrAlreadyExists || r==KErrNone);
	if (r==KErrCorrupt)
		FormatFat(gSessionPath[0]-'A');
	if (r!=KErrNone && r!=KErrAlreadyExists)
		{
		r=TheFs.MkDirAll(gSessionPath);
		test(r==KErrNone);
		}
	}


LOCAL_C TInt ValidateDriveSelection(TDriveUnit aDrive,TSelectedTest aTest)
	{
	if ((aDrive==EDriveZ)||((aDrive==EDriveC)&&(aTest==ELocalDriveTest)))
		{
		
		test.Printf(_L("Test not available for this drive\n"));
		test.Printf(_L("Press any key to continue...\n"));
		test.Getch();
		return (KErrNotSupported);
		}
	else
		return (KErrNone);
	}



LOCAL_C TInt TestLocalDrive(TAny* aSelector)
//
// Test TBusLocalDrive
//
	{

	if (((CSelectionBox*)aSelector)->CurrentKeyPress()!=EKeyEnter)
		return(KErrNone);
	TDriveUnit drive=((CSelectionBox*)aSelector)->CurrentDrive();
	TInt r=ValidateDriveSelection(drive,ELocalDriveTest);
	if (r==KErrNotSupported)
		return (r);
	
	InitializeDrive((CSelectionBox*)aSelector);
//
	TBuf<128> testTitle;
	TBuf<KMaxFileName> name=drive.Name();
	testTitle.Format(_L("Test TBusLocalDrive %S"),&name);
	test.Start(testTitle);
	TestLocalDriveRead(drive,16);
	TestLocalDriveRead(drive,1024);
	TestLocalDriveRead(drive,4096);
//
	TestLocalDriveWrite(drive,16);
	TestLocalDriveWrite(drive,1024);
	TestLocalDriveWrite(drive,4096);
//
	test.Printf(_L("Test finished, reformatting drive %d\n"),gSessionPath[0]-'A');
	FormatFat(gSessionPath[0]-'A');
	test.End();
	return(KErrNone);
	}

LOCAL_C TInt TestFindEntries(TAny* aSelector)
//
// Test Entry
//
	{

	if (((CSelectionBox*)aSelector)->CurrentKeyPress()!=EKeyEnter)
		return(KErrNone);
	
	TInt r=ValidateDriveSelection(((CSelectionBox*)aSelector)->CurrentDrive(),EFindEntryTest);
	if (r==KErrNotSupported)
		return(r);

	InitializeDrive((CSelectionBox*)aSelector);
//
	test.Start(_L("Test Entry"));
	TFileName dirFiftyDeep=_L("\\F32-TST\\");		// root + first directory = 2 directory levels
	for(TInt i=0;i<KCheckDskMaxRecursionLevel-2;i++)	// 0 to 47 = 48 directory levels
		{
		dirFiftyDeep+=_L("X");
		dirFiftyDeep.AppendNum(i);
		dirFiftyDeep+=_L("\\");
		}
	r=TheFs.MkDirAll(dirFiftyDeep);
	test(r==KErrNone || r==KErrAlreadyExists);
//
	TestFindEntryBounce(2);
	TestFindEntryBounce(10);
	TestFindEntryBounce(20);
	TestFindEntryBounce(KCheckDskMaxRecursionLevel-3);
//
	TestFindEntrySearch(1,5);
	TestFindEntrySearch(1,10);
	TestFindEntrySearch(10,15);
	TestFindEntrySearch(10,20);
//
	test.Printf(_L("Test finished, removing directory\n"));
	CFileMan* fMan=CFileMan::NewL(TheFs);
	r=fMan->RmDir(_L("\\F32-TST\\X0\\"));
	test(r==KErrNone);
	delete fMan;
	test.End();
	return(KErrNone);
	}

LOCAL_C TInt TestFileSeek(TAny* aSelector)
//
// Test Seek
//
	{

	if (((CSelectionBox*)aSelector)->CurrentKeyPress()!=EKeyEnter)
		return(KErrNone);
	
	TInt r=ValidateDriveSelection(((CSelectionBox*)aSelector)->CurrentDrive(),EFileSeekTest);
	if (r==KErrNotSupported)
		return (r);
	
	InitializeDrive((CSelectionBox*)aSelector);
//
	test.Start(_L("Test Seek"));
	RFile f;
	gSeekFile=_L("\\F32-TST\\FILE512K.BIG");
	r=f.Replace(TheFs,gSeekFile,EFileWrite);
	test(r==KErrNone);
	r=f.SetSize(524268);
	test(r==KErrNone);
	f.Close();
//
	TestSeek(0,1000);
	TestSeek(0,10000);
	TestSeek(5000,6000);
	TestSeek(5000,15000);
	TestSeek(10000,100000);
	TestSeek(200000,500000);
//
	r=TheFs.Delete(gSeekFile);
	test(r==KErrNone);
	test.End();
	return(KErrNone);
	}

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
//Read the file at position beyond 2GB-1
//Minimum required disk space for this test is 3GB
//
LOCAL_C TInt TestLargeFileSeek(TAny* aSelector)
	{
	
	if (((CSelectionBox*)aSelector)->CurrentKeyPress()!=EKeyEnter)
		return(KErrNone);
	
	TInt r=ValidateDriveSelection(((CSelectionBox*)aSelector)->CurrentDrive(),EFileSeekRFile64Test);
	if (r==KErrNotSupported)
		return (r);
	
	InitializeDrive((CSelectionBox*)aSelector);
//
	test.Start(_L("Test large File Seek"));

	RFile64 BigFile;
	gSeekFile=_L("\\F32-TST\\FILE4GBMINUS2.BIG");
    r=BigFile.Replace(TheFs,gSeekFile,EFileWrite);
	test(r==KErrNone);
    
	//check Disk space
	TVolumeInfo volInfo;
    
    r = TheFs.Volume(volInfo);
    test(r==KErrNone);
	
	//Get the free space available for test
	if(volInfo.iFree < (K3GB-2)) 
		{
		BigFile.Close();
		test(r==KErrNone);
		r=TheFs.Delete(gSeekFile);
		test.Printf(_L("Large File test is skipped: Free space %ld \n"),volInfo.iFree);
		return r;
		}

	r=BigFile.SetSize(K3GB-2);
	test(r==KErrNone);
	BigFile.Close();

	TestSeek64(0,1000);
	TestSeek64(5000,6000);
	TestSeek64(200000,500000);
    TestSeek64(K2GB,K2GB+1000);
	TestSeek64(K3GB-1000,K3GB-2000);
	TestSeek64(K3GB-50, K3GB-20);
	r=TheFs.Delete(gSeekFile);
	test(r==KErrNone);

	test.End();
	return(KErrNone);

	}
#endif //SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

GLDEF_C void CallTestsL()
//
// Call all tests
//
	{

	TInt r=client.CreateLocal(0);
	test(r==KErrNone);
	gSessionPath=_L("?:\\F32-TST\\");
	CSelectionBox* TheSelector=CSelectionBox::NewL(test.Console());
	TCallBack localDrivesCb(TestLocalDrive,TheSelector);
	TCallBack findEntriesCb(TestFindEntries,TheSelector);
	TCallBack fileSeekCb(TestFileSeek,TheSelector);

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TCallBack largefileSeekCb(TestLargeFileSeek,TheSelector);
#endif //SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API

	TheSelector->AddDriveSelectorL(TheFs);
	TheSelector->AddLineL(_L("Test LocalDrive"),localDrivesCb);
	TheSelector->AddLineL(_L("Test Find Entries"),findEntriesCb);
	TheSelector->AddLineL(_L("Test File Seek"),fileSeekCb);

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TheSelector->AddLineL(_L("Test large File Seek"),largefileSeekCb);
#endif //SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TheSelector->Run();
	client.Close();
	delete TheSelector;
	}
