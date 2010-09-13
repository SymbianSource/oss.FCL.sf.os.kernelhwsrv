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
// f32test\bench\t_fsrdirload.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_select.h"
#include "../server/t_server.h"
#include "t_benchmain.h"


GLDEF_D RTest test(_L("File Server Benchmarks, Dir Load"));

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FSRDIRLOAD-0273
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ000
//! @SYMTestCaseDesc    This test case is measuring performance of the FAT implementation.  
//! @SYMTestActions     0.  Expects the files to exist in order to successful execution
//!						1.	Time the listing of each directory sorted by name
//!						2.	Time the listing of each directory sorted by extension
//!						3.	Time the listing of each directory sorted by size
//!						4.	Time the listing of each directory sorted by date
//!						5.	Time the search of each file in a directory sorted by name
//!						6.	Time the search of each file in a directory sorted by extension
//!						7.	Time the search of each file in a directory sorted by size
//!						8.	Time the search of each file in a directory sorted by date
//!
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------


LOCAL_D RSemaphore client;
LOCAL_D TRequestStatus stat;
LOCAL_D TBuf8<4096> buf;

LOCAL_D TDriveList gDriveList;


_LIT(KDirMultipleName2, "dir%d_%d\\");

/** Sort a directory with different criterias 

	@param aN 		Number of files in the directory
	@param aKey		Type of sorting to use
	@param aStep 	Test step
*/
LOCAL_C TInt SortFile(TInt aN, TEntryKey aKey, TInt aStep)
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> temp;
	
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;

	if(aN <= gFilesLimit) 
		{	
		CDir* dirPtr;
			
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;
		
		temp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(temp);
		
		temp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(temp);
		
		temp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(temp);
		
		dir1.Append(_L("*.*"));
		dir2.Append(_L("*.*"));
		dir3.Append(_L("*.*"));
		
		if(gTypes >= 1) 
			{		
			startTime.HomeTime();
			r = TheFs.GetDir(dir1,KEntryAttMaskSupported,ESortNone,dirPtr);
			FailIfError(r);

			r = dirPtr->Sort(aKey);
			FailIfError(r);

			endTime.HomeTime();
			delete dirPtr;
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}

		if(gTypes >= 2) 
			{	
			startTime.HomeTime();
			r = TheFs.GetDir(dir2,KEntryAttMaskSupported,ESortNone,dirPtr);
			FailIfError(r);
			
			r = dirPtr->Sort(aKey);
			FailIfError(r);
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			delete dirPtr;
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{	
			startTime.HomeTime();
			r = TheFs.GetDir(dir3,KEntryAttMaskSupported,ESortNone,dirPtr);
			FailIfError(r);
			
			r = dirPtr->Sort(aKey);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			delete dirPtr;
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
		
	return KErrNone;
	}

/** Sort a directory with different type of files and find a particular one

	@param aN 		Number of files in the directory
	@param aKey		Type of sorting to use
	@param aStep 	Test step
*/
LOCAL_C TInt SortFindFile(TInt aN, TEntryKey aKey, TInt aStep )
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> temp;
	
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;
	
	if(aN <= gFilesLimit) 
		{	
		CDir* dirPtr;
		TInt i = 0;
		TBool found = EFalse;
			
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;
		
		temp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(temp);
		
		temp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(temp);
		
		temp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(temp);
		
		dir1.Append(_L("*.*"));
		dir2.Append(_L("*.*"));
		dir3.Append(_L("*.*"));
		
		if(gTypes >= 1) 
			{	
			startTime.HomeTime();
			r = TheFs.GetDir(dir1, KEntryAttMaskSupported, aKey, dirPtr);
			FailIfError(r);
			
			found = EFalse; 
			i = 0;
			while((i < dirPtr->Count()) && (!found))
				{
				TEntry e = (*dirPtr)[i];
				if(e.iName == _L("last.txt")) found = ETrue;		
				i++;	
				}
			
			endTime.HomeTime();
			delete dirPtr;
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 2) 
			{	
			startTime.HomeTime();
			r = TheFs.GetDir(dir2, KEntryAttMaskSupported, aKey, dirPtr);
			FailIfError(r);
			
			found = EFalse; 
			i = 0;
			while((i < dirPtr->Count()) && (!found))
				{
				TEntry e = (*dirPtr)[i];
				if(e.iName == _L("last.txt")) found = ETrue;
				i++;	
				}

			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			delete dirPtr;
			
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{		
			startTime.HomeTime();
			r = TheFs.GetDir(dir3, KEntryAttMaskSupported, aKey, dirPtr);
			FailIfError(r);
			
			found = EFalse; 
			i = 0;
			while((i<dirPtr->Count()) && (!found))
				{
				TEntry e = (*dirPtr)[i];
				if(e.iName == _L("last.txt")) found = ETrue;
				i++;	
				}
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			delete dirPtr;
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);

	return KErrNone;
	}
	
	
/** Sort a directory and find a file opens it and reads first 4Kb and last 4Kb

	@param aN 		Number of files in the directory
	@param aKey		Type of sorting to use
	@param aStep 	Test step
*/
LOCAL_C TInt SortFindFileAndOpen(TInt aN, TEntryKey aKey, TInt aStep )
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> temp;
	
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;
		
	CDir* dirPtr;
	TInt i = 0, pos = 0;
	TBool found = EFalse;
	RFile file;

	if(aN <= gFilesLimit) 
		{		
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;
		
		temp.Format(KDirMultipleName2, 1, aN);
		dir1.Append(temp);
		
		temp.Format(KDirMultipleName2, 2, aN);
		dir2.Append(temp);
		
		temp.Format(KDirMultipleName2, 3, aN);
		dir3.Append(temp);
		
		dir1.Append(_L("*.*"));
		dir2.Append(_L("*.*"));
		dir3.Append(_L("*.*"));

	
		if(gTypes >= 1) 
			{	
			startTime.HomeTime();
			r = TheFs.GetDir(dir1, KEntryAttMaskSupported, aKey, dirPtr);
			FailIfError(r);
			
			dir1 = gSessionPath;
			temp.Format(KDirMultipleName2, 1, aN);
			dir1.Append(temp);
			dir1.Append(_L("LAST.TXT"));
				
			found = EFalse; 
			i = 0;
			while((i < dirPtr->Count()) && (!found))
				{
				TEntry e = (*dirPtr)[i];
				if(e.iName == _L("LAST.TXT")) 
					{
					found = ETrue;		
					}
				else 
					{
					i++;	
					}
				}
			
			test(found);
			
			file.Open(TheFs, dir1, EFileShareAny|EFileRead);
			pos = 0;
			file.Seek(ESeekStart, pos);
			r = file.Read(buf);
			FailIfError(r);
			pos = -4 * KOneK; // 4 KB before the end of the file
			file.Seek(ESeekEnd, pos);
			r = file.Read(buf);
			FailIfError(r);
			
			endTime.HomeTime();
			delete dirPtr;
			file.Close();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 2) 
			{		
			startTime.HomeTime();
			r = TheFs.GetDir(dir2, KEntryAttMaskSupported, aKey, dirPtr);
			FailIfError(r);
			
			dir2 = gSessionPath;
			temp.Format(KDirMultipleName2, 2, aN); 
			dir2.Append(temp);
			dir2.Append(_L("LAST.TXT"));

			found = EFalse; 
			i = 0;
			while((i < dirPtr->Count()) && (!found))
				{
				TEntry e = (*dirPtr)[i];
				if(e.iName == _L("LAST.TXT")) 
					{
					found = ETrue;
					}
				else 
					{
					i++;	
					}
				}
			
			test(found);
						
			file.Open(TheFs, dir2, EFileShareAny|EFileRead);
			pos = 0;
			file.Seek(ESeekStart, pos);
			r = file.Read(buf);
			FailIfError(r);
			pos = -4 * KOneK;  // 4 KB before the end of the file
			file.Seek(ESeekEnd, pos);
			r = file.Read(buf);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			delete dirPtr;
			file.Close();
			
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}
		
		if(gTypes >= 3) 
			{	
			startTime.HomeTime();
			r = TheFs.GetDir(dir3, KEntryAttMaskSupported, aKey, dirPtr);
			FailIfError(r);
			
			dir3 = gSessionPath;
			temp.Format(KDirMultipleName2, 3, aN);
			dir3.Append(temp);
			dir3.Append(_L("LAST.TXT"));

			found = EFalse; 
			i = 0;
			while((i < dirPtr->Count()) && (!found))
				{
				TEntry e = (*dirPtr)[i];
				if(e.iName == _L("LAST.TXT")) 
					{
					found = ETrue;
					}
				else 
					{
					i++;	
					}
				}
			
			file.Open(TheFs, dir3, EFileShareAny|EFileRead);
			pos = 0;
			file.Seek(ESeekStart, pos);
			r = file.Read(buf);
			FailIfError(r);
			pos = -4 * KOneK;  // 4 KB before the end of the file
			file.Seek(ESeekEnd, pos);
			r = file.Read(buf);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			file.Close();
			
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			delete dirPtr;
			}
		}
	
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);

	return KErrNone;
	}

/** Times the directory listing sorted by name, extension, size and date

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestDirListing(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep;
	
	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Directory listing sorted by name \n"), gTestHarness, gTestCase);
	
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFile(i, ESortByName, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Directory listing sorted by extension\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFile(i, ESortByExt, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Directory listing sorted by size \n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFile(i, ESortBySize, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Directory listing sorted by date \n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFile(i, ESortByDate, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	return KErrNone;
	}

/** Times the operations required to find a file ordering the list of files by orted by name,
	extension, size and date
	
	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestFindFile(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep;
	
	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Find file when sorted by name \n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFile(i, ESortByName, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find file when sorted by extension\n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFile(i, ESortByExt, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Find file when sorted by size \n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFile(i, ESortBySize, testStep++);
			}
		i += 100;
		}
	
	gTestCase++;	
	test.Printf(_L("#~TS_Title_%d,%d: Find file when sorted by date \n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFile(i, ESortByDate, testStep++);
			}
		i += 100;
		}
	
	gTestCase++;
	
	return KErrNone;
	}

/** Times the opening of a file and read of the first 4 Kb when sorted by name, 
	extension, size and date

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestOpenReadFile(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep;
	
	Validate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Open and read file when sorted by name \n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFileAndOpen(i, ESortByName, testStep++);
			}
		i += 100;
		}
	
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Open and read file when sorted by extension\n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFileAndOpen(i, ESortByExt, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Open and read when sorted by size \n"), gTestHarness, gTestCase);

	i=100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFileAndOpen(i, ESortBySize, testStep++);
			}
		i += 100;
		}

	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Open and read file when sorted by date \n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			SortFindFileAndOpen(i, ESortByDate, testStep++);
			}
		i += 100;
		}
	
	gTestCase++;
	return KErrNone;
	}

/** It goes automatically through all the options

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestAll(TAny* aSelector)
	{
	Validate(aSelector);
	
	TestDirListing(aSelector);
	TestFindFile(aSelector);
	TestOpenReadFile(aSelector);
	
	return KErrNone;
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
	gTestHarness = 2; 	
	gTestCase = 1;
	
	PrintHeaders(1, _L("t_fsrdirload. Directory loading"));
		
	if(gMode==0) 
		{ // Manual	
		gSessionPath = _L("?:\\");
		TCallBack createFiles(TestFileCreate,TheSelector);
		TCallBack dirListFile(TestDirListing,TheSelector);
		TCallBack findFile(TestFindFile,TheSelector);
		TCallBack openReadFile(TestOpenReadFile,TheSelector);
		TCallBack allFile(TestAll,TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Create all files"),createFiles);
		TheSelector->AddLineL(_L("Directory listings"),dirListFile);
		TheSelector->AddLineL(_L("Find file in the listing"),findFile);
		TheSelector->AddLineL(_L("Open and read first and last 4KB "),openReadFile);
		TheSelector->AddLineL(_L("Three last options together"),allFile);
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
