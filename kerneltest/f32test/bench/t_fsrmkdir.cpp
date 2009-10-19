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
// f32test\bench\t_fsrmkdir.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_select.h"
#include "t_benchmain.h"


GLDEF_D RTest test(_L("FS Benchmarks, mkdir"));

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FSRMKDIR-0277
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ000
//! @SYMTestCaseDesc    This test case is measuring performance of the FAT implementation
//! @SYMTestActions     0.  Expects the files to exist in order to successful execution
//!						1.	Time the creation of a directory in each directory with  RFs::MkDir
//!						2.	Time the creation of a directory (with  RFs::MkDir) in each directory 
//!							with different clients accessing the directory
//!						3.	Time the creation of a directory (with  RFs::MkDir) in each directory 
//!							with different clients accessing different directories
//!
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

LOCAL_D RSemaphore client,write_screen;
LOCAL_D const TInt KHeapSize = 0x4000;
LOCAL_D TBuf8<4096> buf;

LOCAL_D TDriveList gDriveList;

LOCAL_D TFileName gDelEntryDir;
LOCAL_D TFileName gDelEntryDir2;

// Concurrent thread
RThread gSpeedy;
RThread gSpeedyII;
TInt gT1;
TInt gT2;
TBool gKillMe=EFalse; 

LOCAL_D TInt ThreadCount=0;

_LIT(KDirMultipleName2, "dir%d_%d\\");
_LIT(KNewDir, "new_dir\\");

_LIT(KDeleteMe,"delete%d.me");
_LIT(KDeleteMe2,"blabla%d.rhd");

/** Delete entry in directory

*/
LOCAL_C TInt DeleteEntryAccess2(TAny* )
	{
	RFs fs;
	TInt r = fs.Connect();
	TBuf<100> dirfile;
	TBuf<50> filename;
	RFile file;
	RTest test(_L("test 2")); 

	fs.SetSessionPath(gSessionPath);
	filename.Format(KDeleteMe2, gT2);
	
	dirfile = gDelEntryDir2;
	dirfile.Append(filename);
	
	client.Signal();
	
	FOREVER
		{
			if(!gKillMe)
				{
				r = file.Create(fs, dirfile, EFileShareAny|EFileWrite);
				if(r == KErrAlreadyExists) 
					r=file.Open(fs, dirfile, EFileShareAny|EFileWrite);
				file.Close();
				FailIfError(r);
				
				r = fs.Delete(dirfile);
				if((r != KErrNone) && (r != KErrInUse)) 
					{
					test.Printf(_L("error = %d\n"), r);
					}
				test(r == KErrNone || r == KErrInUse);
			}
		}
	}

/** Delete entry in directory

*/
LOCAL_C TInt DeleteEntryAccess(TAny*)
	{
	RFs fs2;
	TInt r = fs2.Connect();
	TBuf<100> dirfile;
	TBuf<50> filename;
	RFile file2;
	RTest test(_L("test 2")); 
		
	r = fs2.SetSessionPath(gSessionPath);
	filename.Format(KDeleteMe, gT1);
	
	dirfile = gDelEntryDir;
	dirfile.Append(filename);
	
	client.Signal();
	
	FOREVER
		{
		if(!gKillMe)
			{
			r = file2.Create(fs2, dirfile, EFileShareAny|EFileWrite);
			if(r == KErrAlreadyExists) 
				r = file2.Open(fs2, dirfile, EFileShareAny|EFileWrite);
			file2.Close();
			FailIfError(r);
			r = fs2.Delete(dirfile);

			if((r != KErrNone) && (r != KErrInUse)) 
				{
				test.Printf(_L("error = %d\n"), r);
				}

			test(r == KErrNone || r == KErrInUse);
			}
		}
	}


/** Starts two concurrent client sessions in different directories

*/
LOCAL_C void DoTest2(TThreadFunction aFunction)
	{
	gKillMe = EFalse;

	TBuf<20> buf = _L("Speedy");
	buf.AppendNum(ThreadCount++);
	gT1 = ThreadCount;
	TInt r = gSpeedy.Create(buf, aFunction, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

	buf = _L("Speedy");
	buf.AppendNum(ThreadCount++);
	gT2 = ThreadCount;
	r = gSpeedyII.Create(buf, DeleteEntryAccess2, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
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
	gKillMe = ETrue;
	User::After(10000000);
	
	gSpeedy.Kill(KErrNone);
	gSpeedy.Close();	
	
	gSpeedyII.Kill(KErrNone);
	gSpeedyII.Close();	
	}

/** Time the creation of a directory inside each type of directory 

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void MakeDir(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;
	
	TInt r=0;
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
		
		dir1.Append(KNewDir);
		dir2.Append(KNewDir);
		dir3.Append(KNewDir);

		if(gTypes >= 1) 
			{
			dir4.Format(KDirMultipleName, 1, aN);
			startTime.HomeTime();
			
			r = TheFs.MkDir(dir1);
			FailIfError(r);
			
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			TheFs.RmDir(dir1);
			}
		
		if(gTypes >= 2) 
			{
			startTime.HomeTime();

			r = TheFs.MkDir(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			TheFs.RmDir(dir2);
			}
		
		if(gTypes>=3) 
			{
			startTime.HomeTime();

			r = TheFs.MkDir(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			
			TheFs.RmDir(dir3);
			}
		}

	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	}

/** Time the creation of a directory inside each type of directory with multiple threads ongoing

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void MakeDirM(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;
	
	TInt r = 0;
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
			gDelEntryDir = dir1;
			gDelEntryDir2 = dir1;
			
			dir1.Append(KNewDir);
			DoTest2(DeleteEntryAccess);
			
			startTime.HomeTime();
			
			r = TheFs.MkDir(dir1);
			FailIfError(r);
			
			endTime.HomeTime();
			
			DoTestKill();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			
			TheFs.RmDir(dir1);
			}
		
		if(gTypes >= 2) 
			{
			gDelEntryDir = dir2;
			gDelEntryDir2 = dir2;
			dir2.Append(KNewDir);
			
			DoTest2(DeleteEntryAccess);

			startTime.HomeTime();

			r = TheFs.MkDir(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			
			TheFs.RmDir(dir2);
			}
		
		if(gTypes >= 3) 
			{
			gDelEntryDir = dir3;
			gDelEntryDir2 = dir3;
			dir3.Append(KNewDir);
			DoTest2(DeleteEntryAccess);

			startTime.HomeTime();

			r = TheFs.MkDir(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			
			TheFs.RmDir(dir3);
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);

	}

/** Times the creation of a directory 
	Precondition: This test expectsthe drive already filled with the right files
	
	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestMake(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep;
	
	Validate(aSelector);

	test.Printf(_L("#~TS_Title_%d,%d: MkDir, RFs::MkDir\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{	
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			MakeDir(i, testStep++);	
			}
		i += 100;
		}

	gTestCase++;
	return(KErrNone);
	}

/** Tests the creation of a directory with 2 threads accessing the directory

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestMakeMultSame(TAny* aSelector)
	{
	TInt i;
	TInt testStep; 
		
	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: MkDir with mult clients accessing same dir, RFs::MkDir\n"), gTestHarness, gTestCase);
		
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{	
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			MakeDirM(i, testStep++);	
			}
		i += 100;
		}
	
	gTestCase++;
	return(KErrNone);
	}

/** Tests the creation of a directory with 2 threads accessing different directories 
	(the current and one with 300 files)

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestMakeMultDif(TAny* aSelector)
	{
	TInt i = 100;
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	TInt testStep;
	
	Validate(aSelector);

	CreateDirWithNFiles(300,3);		
			
	directory = gSessionPath;
	dirtemp.Format(KDirMultipleName2, 3, 300);
	directory.Append(dirtemp);
	gDelEntryDir2 = directory;

	test.Printf(_L("#~TS_Title_%d,%d: MkDir with mult clients accessing dif dirs, RFs::MkDir\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{	
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			directory = gSessionPath;
			dirtemp.Format(KDirMultipleName2, 2, i);
			directory.Append(dirtemp);
			gDelEntryDir = directory;

			DoTest2(DeleteEntryAccess);

			MakeDir(i, testStep++);	

			DoTestKill();
			}
		i += 100;
		}

	gTestCase++;
	return(KErrNone);
	}

/** Goes automatically through all the options

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestAll(TAny* aSelector)
	{
	Validate(aSelector);
	
 	TestMake(aSelector);
 	TestMakeMultSame(aSelector);
 	TestMakeMultDif(aSelector);

	return(KErrNone);
	}

/** Call all tests

*/
GLDEF_C void CallTestsL()
	{
	TInt r = client.CreateLocal(0);
	FailIfError(r);
	
	gFileSize = 8;
	
	CSelectionBox* TheSelector = CSelectionBox::NewL(test.Console());
	
	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 6; 	
	gTestCase = 1;

	PrintHeaders(1, _L("t_fsrmkdir. Mkdir"));
	
	if(gMode == 0) 
		{ // Manual
		gSessionPath=_L("?:\\");
		TCallBack createFiles(TestFileCreate, TheSelector);
		TCallBack MkDir(TestMake, TheSelector);
		TCallBack makeMultSame(TestMakeMultSame, TheSelector);
		TCallBack makeMultDif(TestMakeMultDif, TheSelector);
		TCallBack makeAll(TestAll, TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Create all files"), createFiles);
		TheSelector->AddLineL(_L("Mkdir "), MkDir);
		TheSelector->AddLineL(_L("Mkdir mult clients same dir "), makeMultSame);
		TheSelector->AddLineL(_L("Mkdir mult clients dif dir"), makeMultDif);
		TheSelector->AddLineL(_L("Execute all options"), makeAll);
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
