// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\bench\t_fsropen.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_select.h"
#include "../server/t_server.h"
#include "t_benchmain.h"

GLDEF_D RTest test(_L("File Server Benchmarks, Open File"));

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FSROPEN-0275
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ000
//! @SYMTestCaseDesc    This test case is measuring performance of the FAT implementation
//! @SYMTestActions     0.  Expects the files to exist in order to successful execution
//!						1.	Time the opening and read of 4 Kb of last.txt file in each directory 
//!						2.	Time the opening and read of 4 Kb of last.txt file in each directory 
//!							with two  clients accessing the directory 
//!						3.	Time the opening and read of 4 Kb of last.txt file in each directory 
//!							with two clients accessing different directories 
//!
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------


LOCAL_D RSemaphore client;
LOCAL_D const TInt KHeapSize=0x6000;
LOCAL_D TBuf8<4096> buf;

LOCAL_D TDriveList gDriveList;
LOCAL_D TFileName gFindEntryDir;
LOCAL_D TBuf<100> gFindDir;

LOCAL_D TFileName gFindEntryDir2;
LOCAL_D TBuf<100> gFindDir2;

// Concurrent thread
RThread gSpeedy;
RThread gSpeedyII;

LOCAL_D TInt ThreadCount=0;

_LIT(KDirMultipleName2, "dir%d_%d\\");

/** Find entry in directory

*/
LOCAL_C TInt FindEntryAccess2(TAny*)
	{
	RFs fs;
	TInt r = fs.Connect();
	RTest test(_L("test 2")); 
	
	r = fs.SetSessionPath(gSessionPath);

	client.Signal();

	FOREVER
		{
		TEntry entry;

		r = fs.Entry(gFindEntryDir2, entry);
		FailIfError(r);
		test.Printf(_L("1"));
		r=fs.Entry(gFindDir2, entry);
		FailIfError(r);
		}
	}
	
/** Starts two concurrent client sessions in the same directory

*/
LOCAL_C void DoTest(TThreadFunction aFunction)
	{
	TBuf<50> buf = _L("Speedy");
	buf.AppendNum(ThreadCount++);
	TInt r = gSpeedy.Create(buf, aFunction, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

	buf = _L("Speedy");
	buf.AppendNum(ThreadCount++);
	r = gSpeedyII.Create(buf, aFunction, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

	gSpeedy.SetPriority(EPriorityLess);
    gSpeedyII.SetPriority(EPriorityLess);
    
	gSpeedy.Resume();
	gSpeedyII.Resume();
	
	client.Wait();
	client.Wait();
	}
		
/** Starts two concurrent client sessions in different directories

*/
LOCAL_C void DoTest2(TThreadFunction aFunction)
	{
	TBuf<50> buf=_L("Speedy");
	buf.AppendNum(ThreadCount++);
	TInt r = gSpeedy.Create(buf, aFunction, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

	buf = _L("Speedy");
	buf.AppendNum(ThreadCount++);
	r = gSpeedyII.Create(buf, FindEntryAccess2, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

 	gSpeedy.SetPriority(EPriorityLess);
    gSpeedyII.SetPriority(EPriorityLess);
    
	gSpeedy.Resume();
	gSpeedyII.Resume();
	
	client.Wait();
	client.Wait();
	}
	
/** Kills the concurrent session

*/
LOCAL_C void DoTestKill()
	{
	gSpeedy.Kill(KErrNone);
	gSpeedy.Close();	
	
	gSpeedyII.Kill(KErrNone);
	gSpeedyII.Close();	
	}

/** Find entry in directory

*/
LOCAL_C TInt FindEntryAccess(TAny*)
	{
	RFs fs;
	TInt r = fs.Connect();
	RTest test(_L("test 2")); 
	
	fs.SetSessionPath(gSessionPath);
	
	client.Signal();
	
	FOREVER
		{
		TEntry entry;
		
		r = fs.Entry(gFindEntryDir, entry);
		FailIfError(r);

		r = fs.Entry(gFindDir,entry);
		FailIfError(r);
		}
	}

/** Find last.txt by opening it and with two threads accessing the current directory 
	and looking for the same file

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void FindFileM2(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
	TBuf16<100> dirtemp;

	TInt r = 0;

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	RFile file;
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1; 

	if(aN <= gFilesLimit) 
		{
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;

		dirtemp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dirtemp);
		gFindDir = dir1;
		
		dirtemp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dirtemp);
			
		dir1.Append(KCommonFile);
		
		if(gTypes >= 1) 
			{
			gFindEntryDir = dir1;
			DoTest(FindEntryAccess);

			User::After(200);
			startTime.HomeTime();
			
			r = file.Open(TheFs, dir1, EFileShareAny|EFileWrite);
			FailIfError(r);

			endTime.HomeTime();
			
			DoTestKill();
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 2) 
			{
			gFindDir = dir2;
			dir2.Append(KCommonFile);
			gFindEntryDir = dir2;
			
			DoTest(FindEntryAccess);
			User::After(200);
			startTime.HomeTime();

			r = file.Open(TheFs, dir2, EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			
			DoTestKill(); 
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{
			gFindDir = dir3;	
			dir3.Append(KCommonFile);
			
			gFindEntryDir = dir3;
			DoTest(FindEntryAccess);

			User::After(200);
			startTime.HomeTime();

			r=file.Open(TheFs, dir3, EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			
			}
		}
		
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Find last.txt by opening it and without any other process

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void OpenFile(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
	TBuf16<100> dirtemp;

	TInt r = 0;
	TInt pos = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1; 
	RFile file;
	
	if(aN <= gFilesLimit) 
		{
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;

		dirtemp.Format(KDirMultipleName, 1, aN);
		dir1.Append(dirtemp);
		
		dirtemp.Format(KDirMultipleName, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName, 3, aN);
		dir3.Append(dirtemp);
			
		dir1.Append(KCommonFile);
		dir2.Append(KCommonFile);
		dir3.Append(KCommonFile);
		
		if(gTypes >= 1) 
			{
			startTime.HomeTime();
			
			r = file.Open(TheFs, dir1, EFileShareAny|EFileRead);
			FailIfError(r);
			r = file.Seek(ESeekStart, pos);
			FailIfError(r);
			r = file.Read(buf);
			FailIfError(r);
			
			endTime.HomeTime();
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			
			}
		
		if(gTypes >= 2) 
			{
			startTime.HomeTime();

			r = file.Open(TheFs, dir2, EFileShareAny|EFileRead);
			FailIfError(r);
			r = file.Seek(ESeekStart,pos);
			FailIfError(r);
			r = file.Read(buf);
			FailIfError(r);
			
			endTime.HomeTime();
			
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{
			startTime.HomeTime();

			r = file.Open(TheFs, dir3, EFileShareAny|EFileRead);
			FailIfError(r);
			r = file.Seek(ESeekStart, pos);
			FailIfError(r);
			r = file.Read(buf);
			FailIfError(r);
			
			endTime.HomeTime();
			file.Close();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);

	}

/** Find last.txt with TFindFile and with two threads accessing the current directory and other one 

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void FindFileMD1(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;
    TBuf16<100> dirtemp;
	
	TInt r = 0;
	TFindFile find(TheFs);
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1; 

	if(aN <= gFilesLimit) 
		{
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;

		dirtemp.Format(KDirMultipleName2,1,aN);
		dir1.Append(dirtemp);
		
		gFindDir = dir1;
		dirtemp.Format(KDirMultipleName, 2, aN);
		dir2.Append(dirtemp);
		
		dirtemp.Format(KDirMultipleName, 3, aN);
		dir3.Append(dirtemp);

		dir4=gSessionPath;
		dirtemp.Format(KDirMultipleName, 3, 300);
		
		dir4.Append(dirtemp);
		gFindDir2 = dir4;
		
		dir1.Append(KCommonFile);
		dir2.Append(KCommonFile);
		dir3.Append(KCommonFile);
		dir4.Append(KCommonFile);
		gFindEntryDir2 = dir4;
		
		if(gTypes >= 1) 
			{

			gFindEntryDir = dir1;
			
			dir4.Format(KDirMultipleName, 1, aN);

			DoTest2(FindEntryAccess);
			startTime.HomeTime();
			
			r = find.FindByPath(dir1, &dir4);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 2) 
			{
			dir4.Format(KDirMultipleName, 2, aN);

			gFindEntryDir=dir2;

			DoTest2(FindEntryAccess);

			startTime.HomeTime();

			r = find.FindByPath(dir2,&dir4);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			
			DoTestKill();
			
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{
			dir4.Format(KDirMultipleName, 3, aN);
			gFindEntryDir = dir3;

			DoTest2(FindEntryAccess);

			startTime.HomeTime();

			r = find.FindByPath(dir3,&dir4);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}


/** Find last.txt by opening the file and with two threads accessing the current directory and other one 

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void FindFileMD2(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
	TBuf16<100> dir4;
	TBuf16<100> dirtemp;

	TInt r = 0;

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1; 

	RFile file;
	
	if(aN <= gFilesLimit) 
		{
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;
		dir4 = gSessionPath;

		dirtemp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dirtemp);
		gFindDir = dir1;
		
		dirtemp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dirtemp);
			
		dir1.Append(KCommonFile);
		gFindEntryDir = dir1;
		
		dirtemp.Format(KDirMultipleName, 3, 300);
		dir4.Append(dirtemp);
		gFindDir2 = dir4;
		dir4.Append(KCommonFile);
		gFindEntryDir2 = dir4;
			
		if(gTypes >= 1)
			{
			DoTest2(FindEntryAccess);
			User::After(200);
		
			startTime.HomeTime();
			
			r = file.Open(TheFs,dir1,EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			
			DoTestKill();
			file.Close();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 2)
			{	
			gFindDir = dir2;
			dir2.Append(KCommonFile);
			gFindEntryDir = dir2;

	 		DoTest2(FindEntryAccess);
			User::After(200);
			
			startTime.HomeTime();
			
			r = file.Open(TheFs, dir2, EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			
			DoTestKill();
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3)
			{
			gFindDir = dir3;	
			dir3.Append(KCommonFile);
			gFindEntryDir = dir3;
			DoTest2(FindEntryAccess);

			User::After(200);
			startTime.HomeTime();

			r = file.Open(TheFs, dir3, EFileShareAny|EFileWrite); 
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Times the system when opening a file with multiple clients accessing the directory
	Precondition: This test expects the drive already filled with files
	
	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestOpenEntry(TAny* aSelector)
	{
	TInt i = 100, r = 0;
	TInt testStep = 1;
	TTime startTime;
	TTime endTime;
	TTimeIntervalSeconds timeTaken;

	Validate(aSelector);

	test.Printf(_L("#~TS_Title_%d,%d: Open and read 4 Kb entry last.txt, RFile::Open \n"), gTestHarness, gTestCase);
	startTime.HomeTime();
	
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			OpenFile(i, testStep++);
		i += 100;
		}

	gTestCase++;
	
	endTime.HomeTime();
	r = endTime.SecondsFrom(startTime, timeTaken);
	FailIfError(r);
	test.Printf(_L("#~TS_Timing_%d,%d=%d\n"), gTestHarness, gTestCase, timeTaken.Int());
	
	return(KErrNone);
	}

/** Times the system when opening a file with multiple clients accessing the directory
	Precondition: This test expects the drive already filled with files

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestOpenEntryMultipleClients(TAny* aSelector)
	{
	TInt i = 100, r = 0;
	TInt testStep;
	TTime startTime;
	TTime endTime;
	TTimeIntervalSeconds timeTaken;

	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Find entry mult clients accessing same directory, RFile::Open \n"), gTestHarness, gTestCase);
	startTime.HomeTime();

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileM2(i, testStep++);
		i += 100;
		}
		
	gTestCase++;
	
	endTime.HomeTime();
	r = endTime.SecondsFrom(startTime, timeTaken);
	FailIfError(r);
	test.Printf(_L("#~TS_Timing_%d,%d=%d\n"), gTestHarness, gTestCase, timeTaken.Int());
	
	return(KErrNone);
	}

/** Times the system when opening a file with multiple clients accessing dif directory
	Precondition: This test expects the drive already filled with files

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestOpenEntryMultipleClientsDD(TAny* aSelector)
	{
	TInt i = 100, r = 0;
	TInt testStep;
	TTime startTime;
	TTime endTime;
	TTimeIntervalSeconds timeTaken;

	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Find entry mult clients accessing dif directory, TFindFile\n"), gTestHarness, gTestCase);
	startTime.HomeTime();
	
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileMD1(i, testStep++);
		i += 100;
		}

	gTestCase++;
	
	endTime.HomeTime();
	r = endTime.SecondsFrom(startTime, timeTaken);
	FailIfError(r);
	test.Printf(_L("#~TS_Timing_%d,%d=%d\n"), gTestHarness, gTestCase, timeTaken.Int());
	
	
	test.Printf(_L("#~TS_Title_%d,%d: Find entry mult clients accessing dif directory, RFile::Open \n"), gTestHarness, gTestCase);
	startTime.HomeTime();
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles) 
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileMD2(i, testStep++);
		i += 100;
		}

	gTestCase++;

	endTime.HomeTime();
	r = endTime.SecondsFrom(startTime, timeTaken);
	FailIfError(r);
	test.Printf(_L("#~TS_Timing_%d,%d=%d\n"), gTestHarness, gTestCase, timeTaken.Int());

	return(KErrNone);
	}

/** It goes automatically through all the options

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestAll(TAny* aSelector)
	{
	Validate(aSelector);
	
	TestOpenEntry(aSelector);
	TestOpenEntryMultipleClients(aSelector);
	TestOpenEntryMultipleClientsDD(aSelector);

	return(KErrNone);
	}

/** Call all tests

*/
GLDEF_C void CallTestsL()
	{
	TInt r = client.CreateLocal(0);
	FailIfError(r);
	
	CSelectionBox* TheSelector = CSelectionBox::NewL(test.Console());

	gFileSize = 8;
	
	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 4; 	
	gTestCase = 1;

	CreateDirWithNFiles(300, 3);
	PrintHeaders(1, _L("t_fsropen. File Open"));
	
	if(gMode == 0) 
		{ // Manual
		gSessionPath=_L("?:\\");
		TCallBack createFiles(TestFileCreate, TheSelector);
		TCallBack openFile(TestOpenEntry, TheSelector);
		TCallBack openFileMC(TestOpenEntryMultipleClients, TheSelector);
		TCallBack openFileMCDD(TestOpenEntryMultipleClientsDD, TheSelector);
		TCallBack allOpen(TestAll,TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Create all files"), createFiles);
		TheSelector->AddLineL(_L("Open random file"), openFile);
		TheSelector->AddLineL(_L("With mult clients same directory"), openFileMC);
		TheSelector->AddLineL(_L("With mult clients dif large directories"), openFileMCDD);
		TheSelector->AddLineL(_L("All options"), allOpen);
		TheSelector->Run();
		}
	else 
		{ // Automatic
		TestAll(TheSelector);
		}
	
	client.Close();
	test.Printf(_L("#~TestEnd_%d\n"), gTestHarness);
	delete TheSelector;
	}
