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
// f32test\bench\t_fsrrepeat.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_select.h"
#include "t_benchmain.h"


GLDEF_D RTest test(_L("FS Benchmarks, Open and read 4 KB"));

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FSRREPEAT-0276
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ000
//! @SYMTestCaseDesc    This test case is measuring performance of the FAT implementation
//! @SYMTestActions     0.  Expects the files to exist in order to successful execution
//!						1.	Time the opening and read 3 times 4 Kb of last.txt file in each directory 
//!						2.	Time the opening and read 3 times 4 Kb of last.txt file in each directory 
//!							with two  clients accessing the directory 
//!						3.	Time the opening and read 3 times of 4 Kb of last.txt file in each directory 
//!							with two clients accessing different directories
//!
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------


LOCAL_D RSemaphore client,write_screen;
LOCAL_D const TInt KHeapSize=0x4000;

LOCAL_D TDriveList gDriveList;

LOCAL_D TFileName gDelEntryDir;
LOCAL_D TFileName gDelEntryDir2;

// Concurrent threads
RThread gSpeedy;
RThread gSpeedyII;
TInt gT1;
TInt gT2;
TBool gKillMe = EFalse; 

LOCAL_D TInt ThreadCount = 0;
LOCAL_D TBuf8<4096> buf;

_LIT(KDirMultipleName2, "dir%d_%d\\");

_LIT(KDeleteMe,"DELETE%d.ME");
_LIT(KDeleteMe2,"BLABLA%d.RHD");


/** Delete entry in directory

*/
LOCAL_C TInt DeleteEntryAccess2(TAny* )
	{
	RFs fs;
	TInt r = fs.Connect();
	TBuf<100> dirfile;
	TBuf<50> filename;
	RFile file;

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
				r = file.Open(fs, dirfile, EFileShareAny|EFileWrite);
			file.Close();
			FailIfError(r);
			
			r = fs.Delete(dirfile);
			if((r != KErrNone) && (r != KErrInUse)) 
				{
				test.Printf(_L("error = %d\n"), r);
				}				
			test((r == KErrNone) || (r == KErrInUse));
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
	
	fs2.SetSessionPath(gSessionPath);
	filename.Format(KDeleteMe,gT1);
	
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
			test((r == KErrNone) || (r == KErrInUse));			 
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

/** Open last.txt with RFs and without any other process

	@param aN 		Number of files in the directory
	@param aType	Type of files 
	@param aStep 	Test step
*/
LOCAL_C void OpenFile(TInt aN, TInt aType, TInt aStep) 
	{
	TBuf16<100> file;
    TBuf16<100> dir;
	
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTakenArray[3] = {-1, -1, -1};
	TInt i;
	RFile f;
	
	if(aN <= gFilesLimit) 
		{
		if(aType <= gTypes)
			{
			file = gSessionPath;
			
			dir.Format(KDirMultipleName2, aType, aN);
			file.Append(dir);

			file.Append(KCommonFile);
				
			i = 0;
			while(i < 3) 
				{	
				startTime.HomeTime();
				
				r = f.Open(TheFs, file, EFileShareAny|EFileRead);
				FailIfError(r);
				r = f.Read(buf);
				FailIfError(r);
				
				f.Close();
				
				endTime.HomeTime();

				timeTaken = endTime.MicroSecondsFrom(startTime);
				timeTakenArray[i++] = I64LOW(timeTaken.Int64() / gTimeUnit);
				}
			}
		}
	
	dir.Format(KDirMultipleName,aType,aN);
	
	PrintResultS(aStep, 1, dir);
	PrintResultTime(aStep, 2, timeTakenArray[0]);
	PrintResultTime(aStep, 3, timeTakenArray[1]);
	PrintResultTime(aStep, 4, timeTakenArray[2]);
	}

/** Times the opening of a file and read operation
	Precondition: This test expects the drive already filled with the right files
	
	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestOpen(TAny* aSelector)
	{
	TInt i = 100, j;
	TInt testStep;

	Validate(aSelector);

	test.Printf(_L("#~TS_Title_%d,%d: Open last.txt and read 4 K repeatedly, RFs::Open\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{	
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			j = 1;
			while(j <= KMaxTypes) 
				{
				OpenFile(i, j, testStep++);
				j++;
				}
			}
		i += 100;
		}
	
	gTestCase++;
	return(KErrNone);
	}

/** Times the opening of a file and read operation with two threads accessing 
	different directories

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestOpenMultSame(TAny* aSelector)
	{
	TInt i = 100,j;
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	TInt testStep;	
	
	Validate(aSelector);

	test.Printf(_L("#~TS_Title_%d,%d: Open last.txt and read 4 K repeatedly with mult clients accessing, RFs::Open\n"), gTestHarness, gTestCase);
		
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{	
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			j = 1;
			while(j <= KMaxTypes) 
				{
				directory = gSessionPath;
				dirtemp.Format(KDirMultipleName2, j, i);
				directory.Append(dirtemp);
				gDelEntryDir = directory;
				gDelEntryDir2 = directory;

				DoTest2(DeleteEntryAccess);

				OpenFile(i, j, testStep++);

				DoTestKill();

				j++;
				}
			}
		i += 100;
		}

	gTestCase++;
	return(KErrNone);
	}

/** Times the opening of a file and read operation with two threads accessing 
	different directories

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestOpenMultDif(TAny* aSelector)
	{
	TInt i = 100,j;
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	TInt testStep;
		
	Validate(aSelector);

	CreateDirWithNFiles(300,3);				
	
	directory = gSessionPath;
	dirtemp.Format(KDirMultipleName2, 3, 300);
	directory.Append(dirtemp);
	
	gDelEntryDir2 = directory;

	test.Printf(_L("#~TS_Title_%d,%d: Open last.txt and read 4 K repeatedly mult clients accessing dif dirs, RFs::Open\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{	
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			j = 1;
			while(j <= KMaxTypes) 
				{
				directory = gSessionPath;
				dirtemp.Format(KDirMultipleName2, j, i);
				directory.Append(dirtemp);
				gDelEntryDir = directory;

				DoTest2(DeleteEntryAccess);

				OpenFile(i, j, testStep++);

				DoTestKill();
				
				j++;
				}
			}
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
	
 	TestOpen(aSelector);
 	TestOpenMultSame(aSelector);
 	TestOpenMultDif(aSelector);

	return(KErrNone);
	}

/** Call all tests

*/
GLDEF_C void CallTestsL()
	{

	TInt r=client.CreateLocal(0);
	FailIfError(r);
	
	gFileSize = 8;

	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 5; 	
	gTestCase = 1;
		
	PrintHeaders(2, _L("t_fsrrepeat. Repeat reading"));

	CSelectionBox* TheSelector = CSelectionBox::NewL(test.Console());
	
	
	if(gMode == 0) 
		{ // Manual
		gSessionPath = _L("?:\\");
		TCallBack createFiles(TestFileCreate,TheSelector);
		TCallBack openF(TestOpen,TheSelector);
		TCallBack openMultSame(TestOpenMultSame,TheSelector);
		TCallBack openMultDif(TestOpenMultDif,TheSelector);
		TCallBack openAll(TestAll,TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Create all files"),createFiles);
		TheSelector->AddLineL(_L("Open and read repeatedly"),openF);
		TheSelector->AddLineL(_L("Same mult clients same dir "),openMultSame);
		TheSelector->AddLineL(_L("Same mult clients dif dir"),openMultDif);
		TheSelector->AddLineL(_L("Execute all options"),openAll);
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
