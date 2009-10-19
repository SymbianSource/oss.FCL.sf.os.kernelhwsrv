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
// f32test\bench\t_fsrdel.cpp
// 
//


#include <f32file.h>
#include <e32test.h>
#include "t_benchmain.h"

GLDEF_D RTest test(_L("File Server Benchmarks, deletion of a massive amount of files"));

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      PBASE-T_FSRDEL-0272
//! @SYMTestType        CIT
//! @SYMPREQ            PREQ000
//! @SYMTestCaseDesc    This test case is measuring performance of the FAT implementation.  
//! @SYMTestActions     0.  Expects the files to exist in order to successful execution
//!						1.	Time the deletion of a file from each directory with RFs::Delete
//!						2.	Time the deletion of a file from each directory with CFileMan::Delete
//!						3.	Time the deletion of all the files from each directory 
//!							with RFs::Delete
//!						4.	Time the deletion of all the files from each directory with 
//!							CFileMan::Delete (wildcard F*.*)
//!						5.	Time the deletion of a file from each directory with RFs::Delete 
//!							with two clients accessing the directory
//!						6.	Time the deletion of a file from each directory with CFileMan::Delete 
//!							with two clients accessing the directory
//!						7.	Time the deletion of all the files from each directory with RFs::Delete 
//!							with two clients accessing the directory
//!						8.	Time the deletion of all the files from each directory with 
//!							CFileMan::Delete with two clients accessing the directory (wildcard F*.*)
//!						9.	Time the deletion of a file from each directory with RFs::Delete 
//!							with two clients accessing different directories 
//!						10.	Time the deletion of a file from each directory with CFileMan::Delete 
//!							with two clients accessing different directories 
//!						11.	Time the deletion of all the files from each directory with RFs::Delete 
//!							with two clients accessing different directories
//!						12.	Time the deletion of all the files from each directory with 
//!							CFileMan::Delete with two clients accessing different directories (wildcard F*.*)
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

LOCAL_D RSemaphore client,write_screen;
LOCAL_D const TInt KHeapSize=0x4000;
LOCAL_D TBuf8<4096> buf;
LOCAL_D TDriveList gDriveList;

// Concurrent thread varibles
RThread gSpeedy;
RThread gSpeedyII;
TInt gT1;
TInt gT2;
LOCAL_D TFileName gDelEntryDir;
LOCAL_D TFileName gDelEntryDir2;
LOCAL_D TInt ThreadCount=0;
TBool gWriting = EFalse;

TBool gKillMe=EFalse; 

_LIT(KDeleteMe,"delete%d.me");
_LIT(KDeleteMe2,"blabla%d.rhd");


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

/** Create background noise by deleting/creating file in gDelEntryDir2 directory

*/
LOCAL_C TInt DeleteEntryAccess2(TAny* )
	{
	RFs fs;
	TInt r = 0;
	TBuf<100> dirfile;
	TBuf<50> filename;
	RFile file;
	RTest test(_L("test 2")); 

	// Not checking error state until main thread has been signalled, to avoid deadlock
	fs.Connect();
	r = fs.SetSessionPath(gSessionPath);
	
	filename.Format(KDeleteMe2,gT2);
	
	dirfile = gDelEntryDir2;
	dirfile.Append(filename);
	
	client.Signal();
	FailIfError(r);
	
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
					FailIfError(r);
				}
		}
	}

/** Create background noise by deleting/creating file in gDelEntryDir directory

*/
LOCAL_C TInt DeleteEntryAccess(TAny*)
	{
	RFs fs2;
	TInt r = 0;
	TBuf<100> dirfile;
	TBuf<50> filename;
	RFile file2;
	RTest test(_L("test 2")); 

	// Not checking error state until main thread has been signalled, to avoid deadlock
	fs2.Connect();
	r = fs2.SetSessionPath(gSessionPath);
	
	filename.Format(KDeleteMe,gT1);
	
	dirfile = gDelEntryDir;
	dirfile.Append(filename);
	
	client.Signal();
	FailIfError(r);
	
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
					FailIfError(r);
				}
		}
	}

/**  Starts a concurrent client session in different directories

*/
LOCAL_C void DoTest(TThreadFunction aFunction)
	{
	gKillMe = EFalse;

	TBuf<20> buffer = _L("Speedy");
	buffer.AppendNum(ThreadCount++);
	gT1 = ThreadCount;
	TInt r = gSpeedy.Create(buffer, aFunction, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

	buffer = _L("Speedy");
	buffer.AppendNum(ThreadCount++);
	gT2=ThreadCount;
	r = gSpeedyII.Create(buffer, DeleteEntryAccess2, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);

 	gSpeedy.SetPriority(EPriorityLess);
    gSpeedyII.SetPriority(EPriorityLess);
	
	gSpeedy.Resume();
	gSpeedyII.Resume();
	
	client.Wait();
	client.Wait();
	}
		
/**   Kills the concurrent sessions

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

/**  Creates again the last.txt file in each directory that is being used

*/
LOCAL_C void ReCreateLast() 
	{
	TInt i = 1, r = 0;
	RFile file;
	TBuf16<50> dirtemp;
	
	TBuf16<50> path;
	TBuf8<1024> dummy(1024);
	
	
	while(i <= gTypes) 
		{
		path = gSessionPath;
		dirtemp.Format(KDirMultipleName, i, 100);
		path.Append(dirtemp);
		path.Append(KCommonFile);
		r = file.Create(TheFs, path, EFileShareAny|EFileWrite);
		if(r == KErrAlreadyExists) 
			{
			r = file.Open(TheFs, path, EFileShareAny|EFileWrite);
			FailIfError(r);
			}
		r = file.Write(dummy);
		FailIfError(r);
		file.Close();
		i++;
		}

	if(gFilesLimit >= 1000) 
		{
		i = 1;
		while(i <= gTypes) 
			{
			path = gSessionPath;
			dirtemp.Format(KDirMultipleName, i, 1000);
			path.Append(dirtemp);
			path.Append(KCommonFile);
			r = file.Create(TheFs, path, EFileShareAny|EFileWrite);
			if(r == KErrAlreadyExists) 
				{
				r = file.Open(TheFs, path, EFileShareAny|EFileWrite);
				FailIfError(r);
				}
			r = file.Write(dummy);
			FailIfError(r);
			file.Close();
			i++;
			}
		}
	
	if(gFilesLimit >= 5000) 
		{
		i = 1;
		while(i <= gTypes) 
			{
			path = gSessionPath;
			dirtemp.Format(KDirMultipleName, i, 5000);
			path.Append(dirtemp);
			path.Append(KCommonFile);
			r = file.Create(TheFs, path, EFileShareAny|EFileWrite);
			if(r == KErrAlreadyExists) 
				{
				r = file.Open(TheFs, path, EFileShareAny|EFileWrite);
				FailIfError(r);
				}
			r = file.Write(dummy);
			FailIfError(r);
			file.Close();
			i++;
			}
		}
	if(gFilesLimit >= 10000) 
		{
		i = 1;
		while(i <= gTypes) 
			{
			path = gSessionPath;
			dirtemp.Format(KDirMultipleName, i, 10000);
			path.Append(dirtemp);
			path.Append(KCommonFile);
			r = file.Create(TheFs, path, EFileShareAny|EFileWrite);
			if(r ==KErrAlreadyExists) 
				{
				r = file.Open(TheFs, path, EFileShareAny|EFileWrite);
				FailIfError(r);
				}
			r = file.Write(dummy);
			FailIfError(r);
			file.Close();
			i++;
			}
		}
	}

/**  Deletes all files in a given directory one by one

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C TInt DelAllFiles1(TInt aN, TInt aStep)
	{
	TInt i = 0, r = 0;
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	
	TBuf16<50> path;
	TBuf16<50> buffer(50); 	

	_LIT(KMsg, "This file caused a problem: %S\n");

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;
	
	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			// all 8.3 
			directory = gSessionPath;
			dirtemp.Format(KDirMultipleName, 1, aN);
			directory.Append(dirtemp);
		
			i = 0;
			startTime.HomeTime();
			while(i < aN) 
				{
				FileNamesGeneration(buffer, 8, i, i%3+1) ;
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				if(r != KErrNone) 
					{
					TBuf16<250> msg;
					msg.Format(KMsg, &path);
					test.Printf(msg);
					FailIfError(r);
					}
				i++;
				}
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}	
		
		if(gTypes >= 2) 
			{
			// all 20.3 chars
			dirtemp.Format(KDirMultipleName, 2, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			
			i = 0;
			startTime.HomeTime();
			while(i < aN) 
				{ 
				FileNamesGeneration(buffer, 20, i, i%3+1) ;
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				if(r != KErrNone) 
					{
					TBuf16<250> msg;
					msg.Format(KMsg, &path);
					test.Printf(msg);
					FailIfError(r);
					}
				i++;
				}	
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		if(gTypes >= 3) 
			{
			// 50/50 
			dirtemp.Format(KDirMultipleName, 3, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			i = 0;
			startTime.HomeTime();
			while(i < aN) 
				{
				if(1 == (i % 2)) 	FileNamesGeneration(buffer, 8, i, i%3+1) ;
				else  				FileNamesGeneration(buffer, 20, i, i%3+1) ;
					
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				if(r != KErrNone) 
					{
					TBuf16<250> msg;
					msg.Format(KMsg, &path);
					test.Printf(msg);
					FailIfError(r);
					}
				i++;
				}
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			}	
		}

	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
				
	return(KErrNone);
	}

/**  Deletes all files in a given directory using wildcards

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C TInt DelAllFiles2(TInt aN, TInt aStep)
	{
	TInt r = 0;
	
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;  
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	CFileMan* fMan=CFileMan::NewL(TheFs);
	

	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			// all 8.3 
			dirtemp.Format(KDirMultipleName, 1, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			
			r = fMan->Delete(directory);
			FailIfError(r);

			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime); 
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		if(gTypes >= 2) 
			{		
			// all 20.3 chars
			dirtemp.Format(KDirMultipleName, 2, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);
			
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		if(gTypes >= 3) 
			{
			// 50/50 
			dirtemp.Format(KDirMultipleName,3, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);
			
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	delete fMan;
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;

	return(KErrNone);
	}

/** Delete last.txt with two threads accessing the current directory and 
	creating/deleting a file with RFs::Delete

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelFileM1(TInt aN, TInt aStep) 
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

	dir1 = gSessionPath;
	dir2 = gSessionPath;
	dir3 = gSessionPath;
	
	dir4.Format(KDirMultipleName, 1, aN);
	dir1.Append(dir4);
	dir4.Format(KDirMultipleName, 2, aN);
	dir2.Append(dir4);	
	dir4.Format(KDirMultipleName, 3, aN);
	dir3.Append(dir4);
	
	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{	
			gDelEntryDir = dir1;
			gDelEntryDir2 = dir1;
			dir1.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);
			
			startTime.HomeTime();

			r = TheFs.Delete(dir1);
			FailIfError(r);
			
			endTime.HomeTime();
			
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 2) 
			{
			gDelEntryDir = dir2;
			gDelEntryDir2 = dir2;
			
			dir2.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = TheFs.Delete(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken=endTime.MicroSecondsFrom(startTime);
				
			DoTestKill();
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 3) 
			{
			gDelEntryDir = dir3;
			gDelEntryDir2 = dir3;
			
			dir3.Append(KCommonFile);

			
			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = TheFs.Delete(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}

/** Delete last.txt by opening it and with two threads accessing the current 
	directory and creating/deleting a file with CFileMan::Delete

	@param aN 		Number of files in the directory
	@param aStep 	Test step 
*/
LOCAL_C void DelFileM2(TInt aN, TInt aStep) 
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

	CFileMan* fMan = CFileMan::NewL(TheFs);

	dir1 = gSessionPath;
	dir2 = gSessionPath;
	dir3 = gSessionPath;
	
	dir4.Format(KDirMultipleName, 1, aN);
	dir1.Append(dir4);
	dir4.Format(KDirMultipleName, 2, aN);
	dir2.Append(dir4);	
	dir4.Format(KDirMultipleName, 3, aN);
	dir3.Append(dir4);

	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			gDelEntryDir = dir1;
			gDelEntryDir2 = dir1;
			
			dir1.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);
			
			startTime.HomeTime();
			
			r = fMan->Delete(dir1);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 2) 
			{
			gDelEntryDir = dir2;
			gDelEntryDir2 = dir2;
			
			dir2.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = fMan->Delete(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		if(gTypes >= 3) 
			{
			gDelEntryDir = dir3;
			gDelEntryDir2 = dir3;
			dir3.Append(KCommonFile);

			
			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = fMan->Delete(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	delete fMan;

	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;

	}

/** Delete all files with two threads accessing the current directory and creating/deleting a file
	with RFs::Delete

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelAllM1(TInt aN, TInt aStep) 
	{
	TInt i = 0, r = 0;
	
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	
	TBuf16<50> path;
	TBuf16<50> buffer(50); 	

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;  
	
	if(aN <= gFilesLimit) 
		{
		// Start the noise in the background
		gDelEntryDir = directory;
		gDelEntryDir2 = directory;
		DoTest(DeleteEntryAccess);
		
		if(gTypes >= 1) 
			{
			// all 8.3 
			dirtemp.Format(KDirMultipleName, 1, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			i = 0;
					
			startTime.HomeTime();
			while(i < aN) 
				{
				FileNamesGeneration(buffer, 8, i, i%3+1) ;
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				FailIfError(r);
				i++;
				}
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		if(gTypes >= 2) 
			{
			// all 20.3 chars
			dirtemp.Format(KDirMultipleName, 2, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			i = 0;
			startTime.HomeTime();
			while(i < aN) 
			 	{
				FileNamesGeneration(buffer, 20, i, i%3+1) ;
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				FailIfError(r);
				i++;
				}	
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		if(gTypes >= 3) 
			{
			// 50/50 
			dirtemp.Format(KDirMultipleName,3, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			
			i = 0;
			startTime.HomeTime();
			while(i < aN) 
				{
				if(1 == (i % 2)) 	FileNamesGeneration(buffer, 8, i, i%3+1) ;
				else  		FileNamesGeneration(buffer, 20, i, i%3+1) ;
					
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				FailIfError(r);
				i++;
				}
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		// Stop the noise in the background
		DoTestKill();
		}

	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}

/** Delete all files with two threads accessing the same directory and creating/deleting a file
	with CFileMan::Delete

	@param aN Number of files in the directory	
	@param aStep 	Test step
*/
LOCAL_C void DelAllM2(TInt aN, TInt aStep) 
	{
	TInt r = 0;
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;  

	CFileMan* fMan = CFileMan::NewL(TheFs);
	
	if(aN <= gFilesLimit) 
		{	
		// Start the noise in the background
		gDelEntryDir = directory;
		gDelEntryDir2 = directory;
		DoTest(DeleteEntryAccess);
		
		if(gTypes >= 1) 
			{
			// all 8.3 
			dirtemp.Format(KDirMultipleName, 1, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			
			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 2) 
			{
			// all 20.3 chars
			dirtemp.Format(KDirMultipleName, 2, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			
			}
		if(gTypes >= 3) 
			{
			// 50/50 
			dirtemp.Format(KDirMultipleName, 3, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);
			endTime.HomeTime();
			
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		// Stop the noise in the background	
		DoTestKill();
		}
	
	delete fMan;
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}


/** Delete last.txt file with two threads accessing different directories and 
	creating/deleting a file in them 

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelFileMD1(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;
    TBuf16<100> temp;
		
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;  


	dir1 = gSessionPath;
	dir2 = gSessionPath;
	dir3 = gSessionPath;
	
	dir4.Format(KDirMultipleName, 1, aN);
	dir1.Append(dir4);
	dir4.Format(KDirMultipleName, 2, aN);
	dir2.Append(dir4);	
	dir4.Format(KDirMultipleName, 3, aN);
	dir3.Append(dir4);


	temp = gSessionPath;
	dir4.Format(KDirMultipleName, 3 ,300);
	temp.Append(dir4);

	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			gDelEntryDir = dir1;
			gDelEntryDir2=temp;
			dir1.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);
			
			startTime.HomeTime();
			
			r = TheFs.Delete(dir1);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 2) 
			{	
			gDelEntryDir = dir2;
			
			dir2.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = TheFs.Delete(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
	
		if(gTypes >= 3) 
			{
			gDelEntryDir = dir3;
			
			dir3.Append(KCommonFile);

			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = TheFs.Delete(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}

/** Delete last.txt file with two threads accessing different directories and 
	creating/deleting a file in them 

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelFileMD2(TInt aN, TInt aStep) 
	{
	TBuf16<100> dir1;
	TBuf16<100> dir2;
	TBuf16<100> dir3;
    TBuf16<100> dir4;
    TBuf16<100> temp;
	
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;  

	CFileMan* fMan=CFileMan::NewL(TheFs);
	
	dir1 = gSessionPath;
	dir2 = gSessionPath;
	dir3 = gSessionPath;
	
	dir4.Format(KDirMultipleName, 1, aN);
	dir1.Append(dir4);
	dir4.Format(KDirMultipleName, 2, aN);
	dir2.Append(dir4);	
	dir4.Format(KDirMultipleName, 3, aN);
	dir3.Append(dir4);
	
	
	temp=gSessionPath;
	dir4.Format(KDirMultipleName, 3, 300);
	temp.Append(dir4);
	
	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			gDelEntryDir = dir1;
			gDelEntryDir2=temp;
			
			dir1.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);
			
			startTime.HomeTime();
			
			r = fMan->Delete(dir1);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 2) 
			{	
			dir4=dir2;
			gDelEntryDir = dir4;

			dir2.Append(KCommonFile);
			
			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = fMan->Delete(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken=endTime.MicroSecondsFrom(startTime);
			DoTestKill();
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		if(gTypes >= 3) 
			{
			dir4=dir3;
			gDelEntryDir = dir4;
		
			dir3.Append(KCommonFile);

			
			DoTest(DeleteEntryAccess);

			startTime.HomeTime();

			r = fMan->Delete(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	
	delete fMan;

	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}

/** Delete all files with two threads accessing different directories and 
	creating/deleting a file in them using RFs::Delete

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelAllMD1(TInt aN, TInt aStep) 
	{
	TInt i = 0,r = 0;
	
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	TBuf16<50> temp;
	TBuf16<50> dir4;


	TBuf16<50> path;
	TBuf16<50> buffer(50); 	

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;  

	
	temp=gSessionPath;
	dir4.Format(KDirMultipleName, 3, 300);
	temp.Append(dir4);

	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			// all 8.3 
			dirtemp.Format(KDirMultipleName,1, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			gDelEntryDir = directory;
			gDelEntryDir2 = temp;

			i = 0;
			
			DoTest(DeleteEntryAccess);
			
			startTime.HomeTime();
			while(i < aN) 
				{
				FileNamesGeneration(buffer, 8, i, i%3+1) ;
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				FailIfError(r);
				i++;
				}
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 2) 
			{
			// all 20.3 chars
			dirtemp.Format(KDirMultipleName,2, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			gDelEntryDir = directory;
			DoTest(DeleteEntryAccess);

			i = 0;
			startTime.HomeTime();
			while(i<aN) 
				{
				FileNamesGeneration(buffer, 20, i, i%3+1) ;
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				FailIfError(r);
				i++;
				}	
			endTime.HomeTime();
			DoTestKill();

			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		if(gTypes >= 3) 
			{
			// 50/50 
			dirtemp.Format(KDirMultipleName,3, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			
			gDelEntryDir = directory;
			DoTest(DeleteEntryAccess);
			i = 0;
			startTime.HomeTime();
			while(i<aN) 
				{
				if(1==(i%2)) 	FileNamesGeneration(buffer, 8, i, i%3+1) ;
				else  		FileNamesGeneration(buffer, 20, i, i%3+1) ;
					
				path = directory;
				path.Append(buffer);
				r = TheFs.Delete(path);
				FailIfError(r);
				i++;
				}
			endTime.HomeTime();
			DoTestKill();

			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}

/** Delete all files with two threads accessing different directories and 
	creating/deleting a file in them using CFileMan::Delete

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelAllMD2(TInt aN, TInt aStep) 
	{
	TInt r = 0;
	TBuf16<50> directory;
	TBuf16<50> dirtemp;
	TBuf16<50> dir4;	
	TBuf16<50> temp;	
	

	TTime startTime;
	TTime endTime;
	TTimeIntervalMicroSeconds timeTaken(0);
	TInt timeTaken1 = -1, timeTaken2 = -1, timeTaken3 = -1;  
	CFileMan* fMan=CFileMan::NewL(TheFs);

	
	// Creating directory for being accessed by other thread
	temp=gSessionPath;
	dir4.Format(KDirMultipleName, 3, 300);
	temp.Append(dir4);
	gDelEntryDir2=temp;

	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			// all 8.3 
			dirtemp.Format(KDirMultipleName,1, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);

			gDelEntryDir = directory;

			DoTest(DeleteEntryAccess);
			
			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);	
			
			endTime.HomeTime();
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			DoTestKill();
			}
		
		if(gTypes >= 2) 
			{
			// all 20.3 chars
			dirtemp.Format(KDirMultipleName,2, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			gDelEntryDir = directory;

			DoTest(DeleteEntryAccess);

			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);

			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		if(gTypes >= 3) 
			{
			// 50/50 
			dirtemp.Format(KDirMultipleName,3, aN);
			directory = gSessionPath;
			directory.Append(dirtemp);
			gDelEntryDir = directory;

			DoTest(DeleteEntryAccess);

			startTime.HomeTime();
			directory.Append(_L("F*.*"));
			r = fMan->Delete(directory);
			FailIfError(r);
			
			endTime.HomeTime();
			DoTestKill();
			
			timeTaken=endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	
	delete fMan;
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}


/** Delete last.txt with RFs::Delete

	@param aN 		Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelFile1(TInt aN, TInt aStep) 
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
	
	dir1 = gSessionPath;
	dir2 = gSessionPath;
	dir3 = gSessionPath;
	
	dir4.Format(KDirMultipleName, 1, aN);
	dir1.Append(dir4);
	dir4.Format(KDirMultipleName, 2, aN);
	dir2.Append(dir4);	
	dir4.Format(KDirMultipleName, 3, aN);
	dir3.Append(dir4);

	dir1.Append(KCommonFile);
	dir2.Append(KCommonFile);
	dir3.Append(KCommonFile);

	if(aN <= gFilesLimit) 
		{
		if(gTypes >= 1) 
			{
			startTime.HomeTime();
			
			r = TheFs.Delete(dir1);
			FailIfError(r);
			
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		if(gTypes >= 2) 
			{
			startTime.HomeTime();

			r = TheFs.Delete(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		if(gTypes >= 3) 
			{
			startTime.HomeTime();

			r = TheFs.Delete(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		}
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}

/** Delete last.txt with CFileMan::Delete

	@param aN Number of files in the directory
	@param aStep 	Test step
*/
LOCAL_C void DelFile2(TInt aN, TInt aStep) 
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
		CFileMan* fMan = CFileMan::NewL(TheFs);
		
		dir1 = gSessionPath;
		dir2 = gSessionPath;
		dir3 = gSessionPath;

		dir4.Format(KDirMultipleName, 1, aN);
		dir1.Append(dir4);
		dir4.Format(KDirMultipleName, 2, aN);
		dir2.Append(dir4);
		dir4.Format(KDirMultipleName, 3, aN);
		dir3.Append(dir4);
			
		dir1.Append(KCommonFile);
		dir2.Append(KCommonFile);
		dir3.Append(KCommonFile);

		if(gTypes >= 1) 
			{
			startTime.HomeTime();
			
			r = fMan->Delete(dir1);
			FailIfError(r);
			
			endTime.HomeTime();

			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken1 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}
		
		if(gTypes >= 2) 
			{
			startTime.HomeTime();

			r = fMan->Delete(dir2);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken2 = I64LOW(timeTaken.Int64() / gTimeUnit );
			}

		if(gTypes >= 3) 
			{
			startTime.HomeTime();

			r = fMan->Delete(dir3);
			FailIfError(r);
			
			endTime.HomeTime();
			timeTaken = endTime.MicroSecondsFrom(startTime);
			timeTaken3 = I64LOW(timeTaken.Int64() / gTimeUnit);
			} 
		
		delete fMan;
		}
	
	gWriting = ETrue; User::After(1000000);
	PrintResult(aStep, 1, aN);
	PrintResultTime(aStep, 2, timeTaken1);
	PrintResultTime(aStep, 3, timeTaken2);
	PrintResultTime(aStep, 4, timeTaken3);
	gWriting = EFalse;
	}

/** Tests the deletion of one file with both API: RFs::Delete and CFileMan::Delete

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestDelEntry(TAny* aSelector)
	{
	// Precondition: the drive already filled with the right files

	TInt i = 100;
	TInt testStep = 1;
	
	Validate(aSelector);

	test.Printf(_L("#~TS_Title_%d,%d: Delete last.txt, RFs::Delete\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)	
			DelFile1(i, testStep++);
		i += 100;
		}
	
	ReCreateLast();
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Delete last.txt, CFileMan::Delete\n"), gTestHarness, gTestCase);
		
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelFile2(i, testStep++);
		i += 100;
		}
	
	gTestCase++;
	
	return(KErrNone);
	}

/** Tests the deletion of all the files in a directory with both API: 
	RFs::Delete and CFileMan::Delete

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestDelAllFiles(TAny* aSelector)
	{
	// Precondition: drive already filled with the right files

	TInt i = 100;
	TInt testStep = 1;
	
	Validate(aSelector);

	test.Printf(_L("#~TS_Title_%d,%d: Delete all, RFs::Delete\n"), gTestHarness, gTestCase);
	
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelAllFiles1(i, testStep++);
		i += 100;
		}

	TestFileCreate(aSelector);
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Delete all, CFileMan::Delete\n"), gTestHarness, gTestCase);	

	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelAllFiles2(i, testStep++);
		i += 100;
		}

	gTestCase++;
	return(KErrNone);
	}

/** Tests the deletion of last.txt in a directory with both API: 
	RFs::Delete and CFileMan::Delete while 2 other threads accessing the directory

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestDelMultSame(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep = 1;


	Validate(aSelector);

	TestFileCreate(aSelector);
	
	test.Printf(_L("#~TS_Title_%d,%d: Delete last.txt mult. clients del in same dir, RFs::Delete\n"), gTestHarness, gTestCase);
	
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelFileM1(i, testStep++);
		i += 100;
		}

	ReCreateLast();
	gTestCase++;	
	test.Printf(_L("#~TS_Title_%d,%d: Delete last.txt mult. clients del in same dir, CFileMan::Delete\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			{
			DelFileM2(i, testStep);
			testStep++;
			}
		i += 100;	
		}

	ReCreateLast();
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Delete all mult. clients del in same dir, RFs::Delete\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelAllM1(i, testStep++);
		i += 100;
		}

	TestFileCreate(aSelector);
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Delete all mult. clients del in same dir, CFileMan::Delete\n"), gTestHarness, gTestCase);

	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelAllM2(i, testStep++);
		i += 100;
		}

	gTestCase++;
	
	return(KErrNone);
	}


/** Tests the deletion of last.txt in a directory with both API: 
	RFs::Delete and CFileMan::Delete while 2 threads accessing different directories 
	(the current and one with 300 files)

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestDelMultDif(TAny* aSelector)
	{
	TInt i = 100;
	TInt testStep = 1;
	
	Validate(aSelector);
	
	TestFileCreate(aSelector);
	CreateDirWithNFiles(300, 3);
	
	test.Printf(_L("#~TS_Title_%d,%d: Delete last.txt mult. clients del in dif dirs, RFs::Delete\n"), gTestHarness, gTestCase);
	
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelFileMD1(i, testStep++);
		i += 100;
		}

	ReCreateLast();
	gTestCase++;	
	test.Printf(_L("#~TS_Title_%d,%d: Delete last.txt mult. clients del in dif dirs, CFileMan::Delete\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelFileMD2(i, testStep++);
		i += 100;
		}

	ReCreateLast();
	gTestCase++;		
	test.Printf(_L("#~TS_Title_%d,%d: Delete all mult. clients del in dif dirs, RFs::Delete\n"), gTestHarness, gTestCase);
	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelAllMD1(i, testStep++);
		i += 100;
		}

	TestFileCreate(aSelector);
	gTestCase++;
	test.Printf(_L("#~TS_Title_%d,%d: Delete all mult. clients del in dif dirs, CFileMan::Delete\n"), gTestHarness, gTestCase);

	
	i = 100;
	testStep = 1;
	while(i <= KMaxFiles)
		{
		if(i == 100 || i == 1000 || i == 5000 || i == 10000)
			DelAllMD2(i, testStep++);
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
	
	gFormat=ETrue; 	// The card will be formatted after this test execution

	TestDelEntry(aSelector);
	TestDelAllFiles(aSelector);
	TestDelMultSame(aSelector);
	TestDelMultDif(aSelector);
	
	return KErrNone;
	}

/** Call all tests

*/
GLDEF_C void CallTestsL()
	{

	TInt r = client.CreateLocal(0);
	FailIfError(r);
	
	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 1; 	
	gTestCase = 1;
	
	PrintHeaders(1, _L("t_fsrdel. Deletion")); 
		
	RThread noisy; 
	TBuf<20> buf = _L("Noisy");
	r = noisy.Create(buf, noise, KDefaultStackSize, KHeapSize, KHeapSize, NULL);
	FailIfError(r);
	
	noisy.Resume();

	CSelectionBox* TheSelector = CSelectionBox::NewL(test.Console());
	
	if(gMode == 0) 
		{ // Manual
		gSessionPath=_L("?:\\");
		TCallBack createFiles(TestFileCreate,TheSelector);
		TCallBack delFile(TestDelEntry,TheSelector);
		TCallBack delAllFiles(TestDelAllFiles,TheSelector);
		TCallBack delMultSame(TestDelMultSame,TheSelector);
		TCallBack delMultDif(TestDelMultDif,TheSelector);
		TCallBack delAll(TestAll,TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Create all files"),createFiles);
		TheSelector->AddLineL(_L("Delete one file from each dir"),delFile);
		TheSelector->AddLineL(_L("Delete all files"),delAllFiles);
		TheSelector->AddLineL(_L("Delete mult clients same dir"),delMultSame);
		TheSelector->AddLineL(_L("Delete mult clients dif dir"),delMultDif);
		TheSelector->AddLineL(_L("Execute all options"),delAll);
		TheSelector->Run();
		}
	else 
		{ // Automatic
		TestAll(TheSelector);
		}
		
	client.Close();
	delete TheSelector;
	
	noisy.Kill(KErrNone);
	noisy.Close();	

	test.Printf(_L("#~TestEnd_%d\n"), gTestHarness);
	}
