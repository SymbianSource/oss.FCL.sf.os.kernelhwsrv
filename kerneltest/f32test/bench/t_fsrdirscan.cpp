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
// f32test\bench\t_fsrdirscan.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_select.h"
#include "../server/t_server.h"
#include "t_benchmain.h"

GLDEF_D RTest test(_L("File Server Benchmarks, DirScan"));

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FSRDIRSCAN-0274
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ000
//! @SYMTestCaseDesc    This test case is measuring performance of the FAT implementation.  
//! @SYMTestActions     0.  Expects the files to exist in order to successful execution
//!						1.	Time finding an entry in each directory with TFindFile
//!						2.	Time finding an entry in each directory with RFile::Open
//!						3.	Time finding an entry in each directory with TFindFile with multiple 
//!							clients accessing the directory
//!						4.	Time finding an entry in each directory with RFile::Open with multiple 
//!							clients accessing the directory
//!						5.	Time finding an entry in each directory with TFindFile with multiple 
//!							clients accessing different directories
//!						6.	Time finding an entry in each directory with RFile::Open with multiple 
//!							clients accessing different directories
//!						7.	Time finding *.txt entries in each directory with TFindFile::FindWildByPath()
//!						8.	Time finding *.txt entries in each directory with TFindFile::FindWildByPath() 
//!							and different clients accessing the same directory
//!						9.	Time finding *.txt entries in each directory with TFindFile::FindWildByPath() 
//!							and different clients accessing different directories
//!						10.	Time finding ffff*.txt entries in each directory with TFindFile::FindWildByPath()
//!						11.	Time finding ffff*.txt entries in each directory with TFindFile::FindWildByPath() 
//!							and different clients accessing the same directory
//!						12.	Time finding ffff*.txt entries in each directory with TFindFile::FindWildByPath() 
//!							and different clients accessing different directories
//!						13.	Time finding last.* entry in each directory with TFindFile::FindWildByPath()
//!						14.	Time finding last.* entry in each directory with TFindFile::FindWildByPath() 
//!							and different clients accessing the same directory
//!						15.	Time finding last.* entry in each directory with TFindFile::FindWildByPath() 
//!							and different clients accessing different directories
//!
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

LOCAL_D RSemaphore client;
LOCAL_D const TInt KHeapSize = 0x4000;
LOCAL_D TBuf8<4096> buf;

LOCAL_D TDriveList gDriveList;

LOCAL_D TFileName gFindEntryDir;
LOCAL_D TBuf<100> gFindDir;

LOCAL_D TFileName gFindEntryDir2;
LOCAL_D TBuf<100> gFindDir2;

// Concurrent threads
RThread gSpeedy;
RThread gSpeedyII;
TInt ThreadCount = 0;

_LIT(KDirMultipleName2, "dir%d_%d\\");

/** Find entry in directory

*/
LOCAL_C TInt FindEntryAccess2(TAny*)
	{
	RFs fs;
	TInt r  = fs.Connect();
	RTest test(_L("test 2")); 
	
	fs.SetSessionPath(gSessionPath);
	
	client.Signal();
	
	FOREVER
		{
			TEntry entry;
			r = fs.Entry(gFindEntryDir2, entry);
			FailIfError(r);
			r = fs.Entry(gFindDir2,entry);
			FailIfError(r);
		}
	}

/** Starts a concurrent client session

	@param aFunction Thread to be started twice
*/
LOCAL_C void DoTest(TThreadFunction aFunction)
	{
	TBuf<20> buf = _L("Speedy");
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
		
/**  Starts a concurrent client session in different directories

	@param aFunction Thread to be started 
*/
LOCAL_C void DoTest2(TThreadFunction aFunction)
	{
	TBuf<20> buf = _L("Speedy");
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
	TInt r = 0;
	
	gSpeedy.Kill(KErrNone);
	FailIfError(r);
	gSpeedy.Close();	
	
	gSpeedyII.Kill(KErrNone);
	FailIfError(r);
	gSpeedyII.Close();	
	}

/** Find entry in directory

*/
LOCAL_C TInt FindEntryAccess(TAny*)
	{
	RFs fs;
	TInt r = fs.Connect();
	RTest test(_L("test 2")); 
	
	r = fs.SetSessionPath(gSessionPath);

	client.Signal();
	
	FOREVER
		{
		TEntry entry;
		
		r = fs.Entry(gFindEntryDir,entry);
		FailIfError(r);
		
		r = fs.Entry(gFindDir,entry);
		FailIfError(r);
		}
	}

/** Find last.txt with TFindFile and with two threads accessing the current directory 
	and looking for the same file

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void FindFileM1(TInt aN, TInt aStep) 
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

		dirtemp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dirtemp);
		gFindDir = dir1;
		
		dirtemp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dirtemp);
			
		dir1.Append(KCommonFile);
		dir2.Append(KCommonFile);
		dir3.Append(KCommonFile);


		if(gTypes >= 1) 
			{	
			gFindEntryDir = dir1;
			DoTest(FindEntryAccess);
			
			dir4.Format(KDirMultipleName, 1, aN);
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
			dir4 = gSessionPath;
			dirtemp.Format(KDirMultipleName, 2, aN);
			dir4.Append(dirtemp);
			gFindDir = dir4;
			gFindEntryDir = dir2;
		
			DoTest(FindEntryAccess);

			startTime.HomeTime();

			r = find.FindByPath(dir2, &dir4);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{	
			dir4 = gSessionPath;
			dirtemp.Format(KDirMultipleName, 3, aN);
			dir4.Append(dirtemp);
	
			gFindDir = dir4;
			gFindEntryDir = dir3;

			DoTest(FindEntryAccess);

			startTime.HomeTime();

			r = find.FindByPath(dir3, &dir4);
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
			
			r = file.Open(TheFs,dir1,EFileShareAny|EFileWrite);
			FailIfError(r);

			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);

			file.Close();
			DoTestKill();
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
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			file.Close();
			DoTestKill();
			
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
			
			r = file.Open(TheFs, dir3, EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			file.Close();
		}
	}

	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Find last.txt with TFindFile and without any other process

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void FindFile1(TInt aN, TInt aStep) 
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

		dirtemp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dirtemp);
		
		dirtemp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dirtemp);
			
		dir1.Append(KCommonFile);
		dir2.Append(KCommonFile);
		dir3.Append(KCommonFile);
		
		r = TheFs.SetSessionPath(gSessionPath);
		FailIfError(r);

		if(gTypes >= 1) 
			{	
			dir4.Format(KDirMultipleName, 1, aN);
			
			startTime.HomeTime();
			
			r = find.FindByPath(dir1, &dir4);
			FailIfError(r);
			
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}

		if(gTypes >= 2) 
			{	
			dir4.Format(KDirMultipleName, 2, aN);
			
			startTime.HomeTime();
			
			r = find.FindByPath(dir2, &dir4);
			FailIfError(r);
			
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}

		if(gTypes >= 3) 
			{	
			dir4.Format(KDirMultipleName, 3, aN);
			
			startTime.HomeTime();
			
			r = find.FindByPath(dir3, &dir4);
			FailIfError(r);
			
			endTime.HomeTime();
			
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
LOCAL_C void FindFile2(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
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

		dirtemp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dirtemp);
		
		dirtemp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dirtemp);
			
		dir1.Append(KCommonFile);
		dir2.Append(KCommonFile);
		dir3.Append(KCommonFile);

		r = TheFs.SetSessionPath(gSessionPath);
		FailIfError(r);
		
		if(gTypes >= 1) 
			{	
			startTime.HomeTime();
			
			r = file.Open(TheFs,dir1,EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			file.Close();
			}
		
		if(gTypes >= 2) 
			{	
			startTime.HomeTime();

			r = file.Open(TheFs, dir2, EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			file.Close();
			
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}

		if(gTypes >= 3) 
			{	
			startTime.HomeTime();

			r = file.Open(TheFs, dir3, EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			file.Close();
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Find last.txt with TFindFile and with two threads accessing the 2 directories

	@param aN Number of files in the directory
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

		dirtemp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dirtemp);

		dirtemp.Format(KDirMultipleName, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName, 3, aN);
		dir3.Append(dirtemp);
		
		dir4 = gSessionPath;
		dirtemp.Format(KDirMultipleName, 3, 300);
		
		dir4.Append(dirtemp);
		
		gFindDir = dir1;
		gFindDir2 = dir4;
		
		dir1.Append(KCommonFile);
		dir2.Append(KCommonFile);
		dir3.Append(KCommonFile);
		dir4.Append(KCommonFile);
		
		gFindEntryDir = dir1;
		gFindEntryDir2 = dir4;
		

		TheFs.SetSessionPath(gSessionPath);
		
		dir4.Format(KDirMultipleName, 1, aN);
		
		if(gTypes >= 1) 
			{	
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
			dir4 = gSessionPath; 
			dirtemp.Format(KDirMultipleName, 2, aN);
			dir4.Append(dirtemp);
			gFindDir = dir4;
			gFindEntryDir=dir2;

			DoTest2(FindEntryAccess);
			dir4.Format(KDirMultipleName, 2, aN);
	
			startTime.HomeTime();

			r = find.FindByPath(dir2,&dir4);
			test(r==KErrNone);
			endTime.HomeTime();
			timeTaken=endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes>=3) 
			{	
			dir4 = gSessionPath; 
			dirtemp.Format(KDirMultipleName, 3, aN);
			dir4.Append(dirtemp);

			gFindDir = dir4;
			gFindEntryDir=dir2;
			
			DoTest2(FindEntryAccess);
			dir4.Format(KDirMultipleName, 3, aN);
		
			startTime.HomeTime();

			r=find.FindByPath(dir3,&dir4);
			test(r==KErrNone);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}


/** Find last.txt by opening the file and with two threads accessing the current 
	directory and other one 

	@param aN Number of files in the directory
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

		dirtemp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dirtemp);
		gFindDir = dir1;
		
		dirtemp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dirtemp);

		dirtemp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dirtemp);
			
		dir1.Append(KCommonFile);
		gFindEntryDir = dir1;
		
		dir4 = gSessionPath;
		dirtemp.Format(KDirMultipleName, 3, 300);
		dir4.Append(dirtemp);

		
		if(gTypes >= 1) 
			{	
			gFindDir2 = dir4;
			dir4.Append(KCommonFile);
			gFindEntryDir2 = dir4;

			DoTest2(FindEntryAccess);

			User::After(200);
			
			startTime.HomeTime();
			
			r = file.Open(TheFs,dir1,EFileShareAny|EFileWrite);
			FailIfError(r);
			
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);

			file.Close();
			DoTestKill();
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
			timeTaken = endTime.MicroSecondsFrom(startTime);
			file.Close();
			DoTestKill();
			
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
			timeTaken=endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			file.Close();
			}
		}

	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** The test finds an entry in each directory
	Precondition: the drive is already filled with the right files

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestFindEntry(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep ;
	
	Validate(aSelector);
	
	
	test.Printf(_L("#~TS_Title_%d,%d: Find entry last.txt, TFindFile\n"), gTestHarness, gTestCase);
	
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFile1(i, testStep++);
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find entry last.txt, RFile::Open \n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFile2(i, testStep++);
		i += 100;
		}

	gTestCase++;
	return(KErrNone);
	}

/** The test finds an entry in each directory with multiple clients accessing the same directory
	Precondition: the drive is already filled with the right files

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestFindEntryMultipleClients(TAny* aSelector)
	{
	TInt i=100;
	TInt testStep;
	
	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Find entry mult clients accessing same directory, TFindFile\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i==1000 || i==5000 || i==10000)
			FindFileM1(i, testStep++);
		i+=100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find entry mult clients accessing same directory, RFile::Open \n"), gTestHarness, gTestCase);
	
	i=100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileM2(i, testStep++);
		i += 100;
		}

	gTestCase++;
	return(KErrNone);
	}

/** The test finds an entry in each directory with multiple clients accessing different directories
	Precondition: the drive is already filled with the right files

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestFindEntryMultipleClientsDD(TAny* aSelector)
	{
	TInt i=100;
	TInt testStep;
	
	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Find entry mult clients accessing dif directory, TFindFile\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileMD1(i, testStep++);
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find entry mult clients accessing dir directory, RFile::Open \n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileMD2(i, testStep++);
		i += 100;
		}

	gTestCase++;
	return(KErrNone);
	}

/** Find last.txt with TFindFile and with two threads accessing the 2 directories

	@param aN 		Number of files in the directory
	@param aWild 	Wildcard string to be used in the search
	@param aStep 	Test step
*/
LOCAL_C void FindFileWild1(TInt aN, const TDesC& aWild, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;

	CDir* dir;
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
		
		dir4.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dir4);
		
		dir4.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dir4);
		
		dir4.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dir4);
		
		if(gTypes >= 1) 
			{	
			dir4.Format(KDirMultipleName, 1, aN);
			startTime.HomeTime();
			
			r = find.FindWildByPath(aWild, &dir1, dir);
			FailIfError(r);
			
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			delete dir;
			}
		
		if(gTypes >= 2) 
			{	
			startTime.HomeTime();

			r = find.FindWildByPath(_L("*.txt"), &dir2, dir);
			FailIfError(r);

			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			delete dir;
			}
		
		if(gTypes >= 3) 
			{	
			startTime.HomeTime();
			
			r = find.FindWildByPath(_L("*.txt"), &dir3, dir);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			delete dir;
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Find last.txt with TFindFile and with two threads accessing the 2 directories

	@param aN 		Number of files in the directory
	@param aWild 	Wildcard string to be used in the search
	@param aStep 	Test step
*/
LOCAL_C void FindFileWild2(TInt aN, const TDesC& aWild, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;
    TBuf16<100> temp;
    
	
	TInt r = 0;
	TFindFile find(TheFs);
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1; 
	
	CDir* dir;
	
	if(aN <= gFilesLimit) 
		{
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;
		
		dir4.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dir4);
		
		dir4.Format(KDirMultipleName2, 2, aN);
		dir2.Append(dir4);
		
		dir4.Format(KDirMultipleName2, 3, aN);
		dir3.Append(dir4);
		
		gFindDir = dir1;
		
		if(gTypes >= 1) 
			{		
			temp = dir1;
			temp.Append(KCommonFile);

			gFindEntryDir = temp;
			DoTest(FindEntryAccess);

			startTime.HomeTime();
			
			r = find.FindWildByPath(aWild, &dir1, dir);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			delete dir;

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 2) 
			{		
			gFindDir = dir2;
			temp = dir2;
			temp.Append(KCommonFile);

			gFindEntryDir = temp;
			

			DoTest(FindEntryAccess);

			startTime.HomeTime();

			r = find.FindWildByPath(aWild, &dir2, dir);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			delete dir;
			
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{	
			gFindDir = dir3;
			temp = dir3;
			temp.Append(KCommonFile);

			gFindEntryDir = temp;
			DoTest(FindEntryAccess);

			startTime.HomeTime();
			
			r = find.FindWildByPath(aWild, &dir3, dir);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			delete dir;
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Find last.txt with TFindFile and with two threads accessing the 2 directories

	@param aN 		Number of files in the directory
	@param aWild 	Wildcard string to be used in the search
	@param aStep 	Test step
*/
LOCAL_C void FindFileWild3(TInt aN, const TDesC& aWild, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;
    TBuf16<100> temp;
    TBuf16<100> temp2;
	
	TInt r = 0;
	TFindFile find(TheFs);
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1; 

	CDir* dir;

	if(aN <= gFilesLimit) 
		{
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;
		
		dir4.Format(KDirMultipleName2, 1, aN);
		dir1.Append(dir4);
			
		temp=gSessionPath;
		dir4.Format(KDirMultipleName, 3, 300);
		
		temp.Append(dir4);
		gFindDir = dir1;
		gFindDir2 = temp;
		
		temp2 = gFindDir;
		temp2.Append(KCommonFile);
		gFindEntryDir = temp2;
		temp2 = gFindDir2;
		temp2.Append(KCommonFile);
		gFindEntryDir2 = temp2;

		if(gTypes >= 1) 
			{	
			DoTest2(FindEntryAccess);
			
			dir4.Format(KDirMultipleName, 1, aN);
			startTime.HomeTime();
			
			r = find.FindWildByPath(aWild, &dir1, dir);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			delete dir;
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 2) 
			{	
			dir4.Format(KDirMultipleName2, 2, aN);
			dir2.Append(dir4);

			temp = dir2;
			temp.Append(KCommonFile);
			gFindDir = dir2;
			gFindEntryDir = temp;

			DoTest2(FindEntryAccess);

			startTime.HomeTime();

			r = find.FindWildByPath(aWild, &dir2, dir);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			delete dir;
	
			timeTaken = endTime.MicroSecondsFrom(startTime);	
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
			
		if(gTypes >= 3) 
			{	
			dir4.Format(KDirMultipleName2, 3, aN);
			dir3.Append(dir4);

			temp = dir3;
			temp.Append(KCommonFile);
			gFindDir = dir3;
			gFindEntryDir = temp;
			
			DoTest2(FindEntryAccess);

			startTime.HomeTime();

			r = find.FindWildByPath(aWild, &dir3, dir);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			delete dir;
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Times the system when looking for files with patterns

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestFileFindPattern(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep;
	
	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Find *.txt, TFindFile::FindWildByPath() only 1 access to the directory \n"), gTestHarness, gTestCase);
	
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild1(i,_L("*.txt"), testStep++);
		i += 100;
		}
		
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find *.txt, TFindFile::FindWildByPath() multiple clients accessing same directory\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild2(i,_L("*.txt"), testStep++);
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find *.txt, TFindFile::FindWildByPath() mult clients access dif directories\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild3(i,_L("*.txt"), testStep++);
		i += 100;
		}
	
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find ffff*.txt, TFindFile::FindWildByPath() only 1 access to the directory \n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild1(i,_L("ffff*.txt"), testStep++);
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find ffff*.txt, TFindFile::FindWildByPath() mult clients access same directory\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild2(i,_L("ffff*.txt"), testStep++);
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find ffff*.txt, TFindFile::FindWildByPath() mult clients access dif directories\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild3(i,_L("ffff*.txt"), testStep++);
		i += 100;
		}
	

	gTestCase++;
	test.Printf(_L("\nFind last.*, TFindFile\n"));
	test.Printf(_L("#~TS_Title_%d,%d: Find last.*, TFindFile::FindWildByPath() only 1 access to the directory \n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i<=KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild1(i,_L("last.*"), testStep++);
		i += 100;
		}
	
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find last.*, TFindFile::FindWildByPath() mult clients access same directory\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild2(i,_L("last.*"), testStep++);
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find last.*, TFindFile::FindWildByPath() mult clients access dif directories\n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			FindFileWild3(i,_L("last.*"), testStep++);
		i += 100;
		}
	
	gTestCase++;
	return(KErrNone);
	}

/** It goes automatically through all the options

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestAll(TAny* aSelector)
	{
	Validate(aSelector);

	TestFindEntry(aSelector);
	TestFindEntryMultipleClients(aSelector);
	TestFindEntryMultipleClientsDD(aSelector);
	TestFileFindPattern(aSelector);

	return(KErrNone);
	}

/** Call all tests

*/
GLDEF_C void CallTestsL()
	{

	TInt r = client.CreateLocal(0);
	FailIfError(r);
	
	CSelectionBox* TheSelector = CSelectionBox::NewL(test.Console());
	
	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 3; 	
	gTestCase = 1;
	
	CreateDirWithNFiles(300, 3);
	PrintHeaders(1, _L("t_fsrdirscan. Directory scanning"));
	
	if(gMode==0) 
		{ // Manual	
		gSessionPath=_L("?:\\");
		TCallBack createFiles(TestFileCreate,TheSelector);
		TCallBack findFile(TestFindEntry,TheSelector);
		TCallBack findFileMC(TestFindEntryMultipleClients,TheSelector);
		TCallBack findFileMCDD(TestFindEntryMultipleClientsDD,TheSelector);
		TCallBack findFilePattern(TestFileFindPattern,TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Create all files"),createFiles);
		TheSelector->AddLineL(_L("Find filename"),findFile);
		TheSelector->AddLineL(_L("Find with mult clients same directory"),findFileMC);
		TheSelector->AddLineL(_L("Find with mult clients dif directories"),findFileMCDD);
		TheSelector->AddLineL(_L("All using glob patterns"),findFilePattern);
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
