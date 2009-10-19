// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\fsstress\t_soak1.cpp
// Suggestions for future improvements: The createVerifyFileX method uses local 
// RBufs for writing and reading. By making these buffers global (at least the 
// write buffers), the overhead of allocating and filling memory can be avoided.
// 
//

#include <f32file.h>
#include <e32test.h>
#include "t_chlffs.h"
#include "t_server.h"
#define __TESTING_LFFS__

LOCAL_D TInt gDriveNumber;

const TInt KMaxNumberThreads     = 4;
const TInt KHeapSize             = 0x600000;
const TInt KDiskUnitThreshold    = 0x1000000; // if disk metrics are above threshold, display in MB rather than KB
const TInt KNotificationInterval = 10 * 1000000;

const TInt KMaxSizeArray         = 3;
const TInt KMaxFilesPerDirectory = 100;

struct TTestMetrics
	{
	TInt SizeArray[KMaxSizeArray][5];
	TInt KSpaceRequiredForMakeAndDelete;
	TInt KFillDiskFileSize;
	};




///#define SINGLE__THREAD

// 256 sectors, 17 sectors+1, 3 bytes, 254 sectors+4, 100 sectors
//  50 sectors, 113 sectors+25, 10 sectors, 103 bytes, 199 sectors+44
// 24 sectors+166, 189 bytes, 225 sectors, 4 sectors+221, 117 sectors+40
LOCAL_D const TTestMetrics DefMetrics =
				{
				{{65536,4353,3,65028,25600},	// Sizearray[0]
				{12800,28953,2560,103,50988},	// Sizearray[1]
				{6310,189,57600,1245,29992}},	// Sizearray[2]
				655360,							// KSpaceRequiredForMakeAndDelete
				65536							// KFillDiskFileSize
				};

// 16 sectors, 1 sectors+1, 3 bytes, 15 sectors+4, 6 sectors
//  3 sectors, 7 sectors+25, 1 sector, 103 bytes, 12 sectors+44
// 2 sectors+166, 189 bytes, 15 sectors, 4 sectors+221, 7 sectors+40
LOCAL_D const TTestMetrics LffsMetrics = 
				{
				{{4096,257,3,3844,1536},		// Sizearray[0]
				{768,1817,256,103,3116},		// Sizearray[1]
				{678,189,3840,1245,1832}},		// Sizearray[2]
				36864,							// KSpaceRequiredForMakeAndDelete
				4096							// KFillDiskFileSize
				};

// 1600 sectors, 100 sectors+100, 1 sector+44, 1501 sectors+144, 600 sectors
// 300 sectors, 709 sectors+196, 100 sectors, 40 sectors+60, 1217 sectors+48
// 264 sectors+216, 73 sectors+212, 486 sectors+84, 715 sectors+160
LOCAL_D const TTestMetrics FAT32Metrics =
				{
				{{409600,25700,300,384400,153600},		// Sizearray[0]
				{76800,181700,25600,10300,311600},		// Sizearray[1]
				{67800,18900,384000,124500,183200}},	// Sizearray[2]
				3686400,								// KSpaceRequiredForMakeAndDelete
				409600									// KFillDiskFileSize
				};

// 8 sectors, 1 sectors+1, 3 bytes, 7 sectors+4, 3 sectors
//  1 sectors, 3 sectors+25, 1 sector, 103 bytes, 6 sectors+44
// 1 sector+166 bytes, 189 bytes, 7 sectors, 2 sectors+221, 3 sectors+40

LOCAL_D const TTestMetrics NandMetrics = 
				{
				{{2048,257,3,1796,768},			// Sizearray[0]
				{256,793,256,103,1580},			// Sizearray[1]
				{422,189,1792,833,808}},		// Sizearray[2]
				20480,							// KSpaceRequiredForMakeAndDelete
				2048							// KFillDiskFileSize
				};



LOCAL_D const TTestMetrics* TheMetrics = 0;


#if defined(__WINS__)
#define WIN32_LEAN_AND_MEAN
#pragma warning (disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (default:4201) // warning C4201: nonstandard extension used : nameless struct/union
#endif

#if defined(_UNICODE)
	#if !defined(UNICODE)
		#define UNICODE
	#endif
#endif

#ifndef SINGLE__THREAD
	#define REUSE_THREAD
#endif

#ifdef REUSE_THREAD
	LOCAL_D TBool gRequestEnd;
#endif

class TThreadTestInfo
	{
public:
	TInt iCycles;
	TInt iErrors;
	TInt iSizeArrayPos;
	TInt iErrorInfo;
	};

GLDEF_D RTest test(_L("T_SOAK1"));
LOCAL_D TThreadTestInfo ThreadTestInfo[KMaxNumberThreads];
LOCAL_D TBool CurrentlyFillingDisk;
LOCAL_D TInt FillDiskCount;
LOCAL_D TInt FillDiskCycle;

LOCAL_C TInt MakeAndDeleteFilesThread(TAny* anId);
LOCAL_C TInt FillAndEmptyDiskThread(TAny* anId);
LOCAL_C TInt CreateVerifyFileX(const TDesC& aBaseName,TInt aSize,RFs &aFs,TInt aPattern);
LOCAL_C TInt MakeFileName(TInt aThreadId, TInt aFileNumber, RFs & aFs, TFileName& aName, TBool aMakeDir = ETrue);
LOCAL_C TInt AppendPath(TFileName& aPath, TInt aNumber, TInt aLevel);

_LIT( KConnect, "Connect" );
_LIT( KDelete, "Delete" );
_LIT( KDriveToChar, "DriveToChar" );
_LIT( KAppendPath, "AppendPath" );
_LIT( KSetSessPath, "SetSessPath" );
_LIT( KMdAll, "MkdirAll" );
_LIT( KVolInfo, "VolInfo" );
_LIT( KReplace, "Replace" );
_LIT( KRead, "Read" );
_LIT( KWrite, "Write" );
_LIT( KFlush, "Flush" );
_LIT( KDataCompare, "DataCompare" );
_LIT( KMemory, "Memory" );
_LIT( KFilePrefix, "FILE_" );

_LIT(KPath, "?:\\SOAK_TEST\\SESSION%d\\");

LOCAL_C void LogError( TInt aError, const TDesC& aAction, const TDesC& aFileName, TUint a1, TUint a2, TInt line = -1 )
	{
	if(line >= 0)
		{
		_LIT(KFmt, "TSOAK1 ERROR (line %d): %d, file [%S], %S, (0x%x, 0x%x)");
		RDebug::Print(KFmt, line, aError, &aFileName, &aAction, a1, a2);
		}
	else 
		{
		_LIT(KFmt, "TSOAK1 ERROR: %d, file [%S], %S, (0x%x, 0x%x)");
		RDebug::Print(KFmt, aError, &aFileName, &aAction, a1, a2);
		}
	}

LOCAL_C TInt MakeFileName(TInt aThreadId, TInt aFileNumber, RFs & aFs, TFileName& aName, TBool aMakeDir)
//
// creates a file name and makes all the directory components, if required
//
	{
	
	TFileName path;
	path.Format(KPath, aThreadId);
	
	TChar driveLetter;
	TInt  r;
	r = aFs.DriveToChar(gDriveNumber, driveLetter);
	if (r != KErrNone)
		{
		LogError(r, KDriveToChar, KNullDesC, driveLetter, 0);
		aFs.Close();
		return(r);
		}
		
	path[0] = (TText) driveLetter;
	r = aFs.SetSessionPath(path);
	if (r != KErrNone)
		{
		LogError(r, KSetSessPath, path, 0, 0);
		aFs.Close();
		return(r);
		}
		
	// add additional directories
	TInt fileNumber;
	fileNumber = aFileNumber;
	r = AppendPath(path, fileNumber, 0);
	if(r != KErrNone)
		{
		LogError(r, KAppendPath, path, fileNumber, 0);
		aFs.Close();
		return(r);
		}
		
	if(aMakeDir)
		{
		r = aFs.MkDirAll(path);
		if (r != KErrNone && r != KErrAlreadyExists)
			{
			LogError(r, KMdAll, path, 0, 0);
			aFs.Close();
			return(r);
			}
		}
		
	// and finally add file name
	path.Append(KFilePrefix);
	path.AppendNum(aFileNumber);

	aName = path;
	return(KErrNone);
	}
	

LOCAL_C TInt AppendPath(TFileName& aPath, TInt aNumber, TInt aLevel)
//
// helper function for MakeFileName()
//
	{
	
	if(aNumber > KMaxFilesPerDirectory)
		{
		aNumber /= KMaxFilesPerDirectory;
		AppendPath(aPath, aNumber, aLevel + 1);
		}
		
	if(aLevel)
		{	
		aPath.AppendNum(aNumber % KMaxFilesPerDirectory);	
		aPath.Append('\\');	
		}
	
	return(KErrNone);
	}

LOCAL_C TInt MakeAndDeleteFilesThread(TAny* anId)
//
// The entry point for the 'MakeAndDeleteFiles' thread.
//
	{

	TInt thrdId=(TInt)anId;
	TInt pattern=(ThreadTestInfo[thrdId].iCycles)%2;
    TInt r;
	RFs f;
	r=f.Connect();
	if (r!=KErrNone)
		{
		LogError( r, KConnect, KNullDesC, 0, 0 );
		ThreadTestInfo[thrdId].iErrorInfo=1;
		return(r);
		}

	TFileName fileName;
	TInt sizeArrayPos = ThreadTestInfo[thrdId].iSizeArrayPos;
#ifdef REUSE_THREAD
	while(!gRequestEnd)
		{
#endif	
		for(TInt i = 0; i < 5; i++)
			{
			// create files
			r = MakeFileName(thrdId, i, f, fileName);
			if(r != KErrNone)
				{
				ThreadTestInfo[thrdId].iErrorInfo=2;
				f.Close();
				return(r);
				}
			
			r = CreateVerifyFileX(fileName, TheMetrics->SizeArray[sizeArrayPos][0], f, pattern);
			if (r!=KErrNone)
				{
				ThreadTestInfo[thrdId].iErrorInfo=3;
				f.Close();
				return(r);
				}
			
			// delete selected files at certain points in the cycle
			TInt deleteFile = EFalse;
			switch(i)
				{
				default:
					// nothing to be done
				break;
				case 2:
					// delete file 0
					deleteFile = ETrue;
					r = MakeFileName(thrdId, 0, f, fileName, EFalse);
				break;
				case 3:
					// delete file 1
					deleteFile = ETrue;
					r = MakeFileName(thrdId, 1, f, fileName, EFalse);
				break;
				}
				
				if(deleteFile)
					{
					// check MakeFileName() error code
					if(r != KErrNone)
						{
						ThreadTestInfo[thrdId].iErrorInfo = 4;
						f.Close();
						return(r);
						}
						
					// and delete file
					r = f.Delete(fileName);
					if(r != KErrNone)
						{
						ThreadTestInfo[thrdId].iErrorInfo = 5;
						f.Close();
						return(r);
						}
					}
			}
			
		sizeArrayPos++;
		if(sizeArrayPos >= KMaxSizeArray)
			{
			sizeArrayPos = 0;
			}
			
		ThreadTestInfo[thrdId].iSizeArrayPos=sizeArrayPos;
#ifdef REUSE_THREAD
	ThreadTestInfo[thrdId].iCycles++;
	pattern = (ThreadTestInfo[thrdId].iCycles) % 2;
	}
#endif

	f.Close();
	return(KErrNone);
	}

LOCAL_C TInt FillAndEmptyDiskThread(TAny* anId)
//
// The entry point for the 'FillAndEmptyDisk' thread.
//
	{

	TInt thrdId=(TInt)anId;
	TInt pattern=(ThreadTestInfo[thrdId].iCycles)%2;
    TInt r;
	RFs f;
	r=f.Connect();
	if (r!=KErrNone)
		{
		LogError( r, KConnect, KNullDesC, 0, 0 );
		ThreadTestInfo[thrdId].iErrorInfo=6;
		return(r);
		}

	TInt i;
#ifdef REUSE_THREAD
	while(!gRequestEnd)
		{
#endif	
		for (i=0;i<5;i++) // Create/Delete 5 files each time
			{
			if (CurrentlyFillingDisk)
				{
				TVolumeInfo v;
				r=f.Volume(v,gDriveNumber);
				if (r!=KErrNone)
					{
					ThreadTestInfo[thrdId].iErrorInfo=7;
					LogError( r, KVolInfo, KNullDesC, gDriveNumber, 0);
					f.Close();
					return(r);
					}
				if (v.iFree<=(TheMetrics->KSpaceRequiredForMakeAndDelete+TheMetrics->KFillDiskFileSize))
					CurrentlyFillingDisk=EFalse;
				else
					{
					TFileName fileName;
					r = MakeFileName(thrdId, FillDiskCount, f, fileName);
					if(r != KErrNone)
						{
						ThreadTestInfo[thrdId].iErrorInfo = 8;
						f.Close();
						return(r);
						}			
					
					r = CreateVerifyFileX(fileName, TheMetrics->KFillDiskFileSize, f, pattern);
					if (r!=KErrNone)
						{
						ThreadTestInfo[thrdId].iErrorInfo=9;
						f.Close();
						return(r);
						}
					FillDiskCount++;
					}
				}
			else
				{
				if (FillDiskCount<=0)
					{
					CurrentlyFillingDisk=ETrue;
					FillDiskCount=0;
					FillDiskCycle++;
					}
				else
					{
					FillDiskCount--;
					TFileName fileName;
					r = MakeFileName(thrdId, FillDiskCount, f, fileName, EFalse);
					if(r != KErrNone)
						{
						ThreadTestInfo[thrdId].iErrorInfo = 10;
						f.Close();
						return(r);
						}
					
					r = f.Delete(fileName);				
					if (r!=KErrNone)
						{
						ThreadTestInfo[thrdId].iErrorInfo=11;
						LogError(r, KDelete, fileName,FillDiskCount, 0);
						f.Close();
						return(r);
						}
					}
				}
			}
#ifdef REUSE_THREAD
		ThreadTestInfo[thrdId].iCycles++;
		pattern = (ThreadTestInfo[thrdId].iCycles) % 2;
		}			
#endif

	f.Close();
	return(KErrNone);
	}

const TInt KCreateFileBufSize = 0x80000; 
LOCAL_C TInt CreateVerifyFileX(const TDesC& aFileName, TInt aSize, RFs& aFs, TInt aPattern)
//
// Create and verify a file.
//
	{
	// Note, the directory structure is provided by MakeFileName(). Hence it 
	// is assumed at this point that the path to the file exists already.
	RFile file;
	TInt r;
	r = file.Replace(aFs, aFileName, EFileWrite);
	if (r!=KErrNone)
		{
		LogError( r, KReplace, aFileName, EFileWrite, 0 );
		return(r);
		}

	// Grow it to the specified size by writing a pattern buffer to it
	// Alternate the pattern buffer each time
	RBuf8 wBuf;
	r = wBuf.CreateMax(KCreateFileBufSize);
	if(r != KErrNone)
		{
		LogError(r, KMemory, aFileName, 0, 0, __LINE__);
		wBuf.Close();
		file.Close();
		return(r);
		}
		
	TInt i;
	
	if (aPattern)
		{
		// ascending pattern
		for (i = 0; i < KCreateFileBufSize; i++)
			{			
			wBuf[i] = (TUint8) i;			
			}
		}
	else
		{
		// descending pattern
		for (i = 0; i < KCreateFileBufSize; i++)
			{
			wBuf[i] = (TUint8) ((KCreateFileBufSize - 1) - i);
			}
		}


	TInt pos;
	TInt chunkSize;
	TInt sectorCount = 0;
	
	for (pos = 0; pos < aSize; pos += chunkSize)
		{
		wBuf[0]=(TUint8)i;	// Insert sector count
		chunkSize = Min((aSize-pos), KCreateFileBufSize);
		r = file.Write(pos, wBuf, chunkSize);
		if (r != KErrNone)
			{
			LogError(r, KWrite, aFileName, pos, chunkSize, __LINE__);
			file.Close();
			wBuf.Close();
			return(r);
			}
			
		sectorCount++;
		}

	// Flush it
	r=file.Flush();
	if (r!=KErrNone)
		{
		LogError( r, KFlush, aFileName, 0, 0, __LINE__);
		file.Close();
		wBuf.Close();
		return(r);
		}

//	Test still works if this is commented out just doesn't verify
	// Read back and verify it
	RBuf8 rBuf;
	r = rBuf.CreateMax(KCreateFileBufSize);
	if(r != KErrNone)
		{
		LogError( r, KMemory, aFileName, 0, 0, __LINE__);
		file.Close();
		wBuf.Close();
		rBuf.Close();
		return(KErrGeneral);
		}
	
	
	for (pos = 0;pos < aSize; pos += chunkSize)
		{
		chunkSize = Min((aSize-pos), KCreateFileBufSize);
		r = file.Read(pos, rBuf, chunkSize);
		if (r != KErrNone)
			{
			LogError(r, KRead, aFileName, pos, 0, __LINE__);
			file.Close();
			wBuf.Close();
			rBuf.Close();
			return(r);
			}
			
		wBuf[0] = (TUint8) i; // Insert sector count
		wBuf.SetLength(chunkSize);
		r = rBuf.Compare(wBuf);
		if (r != 0)
			{
			LogError(r, KDataCompare, aFileName, 0, 0, __LINE__);
			file.Close();
			wBuf.Close();
			rBuf.Close();
			return(KErrGeneral);
			}
		}
//

	file.Close();
	wBuf.Close();
	rBuf.Close();
	return(KErrNone);
	}
	

#ifdef SINGLE__THREAD
LOCAL_C void DoTests()
//
//  single thread
//
    {

	TInt r=KErrNone;
	test.Next(_L("Start continuous file Write/Read/Verify operation"));
	RThread t[KMaxNumberThreads];
	TRequestStatus tStat[KMaxNumberThreads];

	TInt i=0;

	TName threadName;
	TRequestStatus kStat=KRequestPending;
	test.Console()->Read(kStat);
	ThreadTestInfo[i].iCycles=0;
	ThreadTestInfo[i].iErrors=0;
	ThreadTestInfo[i].iSizeArrayPos=(i%KMaxSizeArray);
	ThreadTestInfo[i].iErrorInfo=0;
	if (i<(KMaxNumberThreads-1))
		{
		threadName.Format(_L("MakeAndDeleteFiles%d"),i);
    	r=t[i].Create(threadName,MakeAndDeleteFilesThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)i);
		}
	else
		{
		// Last thread fills/empties disk
		threadName.Format(_L("FillAndEmptyDisk%d"),i);
    	r=t[i].Create(threadName,FillAndEmptyDiskThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)i);
		}
	if (r!=KErrNone)
		test.Printf(_L("Error(%d) creating thread(%d)\r\n"),r,i);
	test(r==KErrNone);
	t[i].Logon(tStat[i]);
	t[i].Resume();
	CurrentlyFillingDisk=ETrue;
	FillDiskCount=0;

    TInt totalTime=0;
    TTime startTime;
    TTime time;
    startTime.UniversalTime();

	TInt ypos=test.Console()->WhereY();
	FOREVER
		{
		User::WaitForAnyRequest();
		if (kStat!=KRequestPending)
			{
    		t[i].LogonCancel(tStat[i]);
			User::WaitForRequest(tStat[i]);
			break;
			}
		else
			{
			TBool threadFinished=EFalse;
			if (tStat[i]!=KRequestPending && !threadFinished)
				{
				t[i].Close();
				(ThreadTestInfo[i].iCycles)++;
				if (tStat[i]!=KErrNone)
					(ThreadTestInfo[i].iErrors)++;
				threadFinished=ETrue;

				// Launch another thread
				TInt threadNameId=((ThreadTestInfo[i].iCycles)%2)?(i+KMaxNumberThreads):i; // Alternate thread name
				threadName.Format(_L("FillAndEmptyDisk%d"),threadNameId);
   				r=t[i].Create(threadName,FillAndEmptyDiskThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)i);
				if (r!=KErrNone)
					test.Printf(_L("Error(%d) creating thread(%d)\r\n"),r,i);
    			test(r==KErrNone);
    			t[i].Logon(tStat[i]);
				t[i].Resume();
				}
				
			test.Console()->SetPos(0,(ypos+i));
			test.Printf(_L("Thread(%d): % 4d errors in % 4d cycles (%d)\r\n"),i,ThreadTestInfo[i].iErrors,ThreadTestInfo[i].iCycles,ThreadTestInfo[i].iErrorInfo);
			if (!threadFinished)
				{
				test.Printf(_L("Semaphore death"));
				break;
				}
			TVolumeInfo v;
			r=TheFs.Volume(v,gDriveNumber);
			test(r==KErrNone);
			test.Console()->SetPos(0,(ypos+KMaxNumberThreads));
			test.Printf(_L("Free space on disk: %u K(of %u K)\r\n"),(v.iFree/1024).Low(),(v.iSize/1024).Low());

            TTimeIntervalSeconds timeTaken;
            time.UniversalTime();
            r=time.SecondsFrom(startTime,timeTaken);
            test(r==KErrNone);
            totalTime=timeTaken.Int();

            TInt seconds = totalTime % 60;
            TInt minutes = (totalTime / 60) % 60;
            TInt hours   = (totalTime / 3600) % 24;
            TInt days    = totalTime / (60 * 60 * 24);
            test.Printf(_L("Elapsed Time: %d d %02d:%02d:%02d\r\n"), days, hours, minutes, seconds);
			}
		}
    }

#else 

LOCAL_C void DoTests()
//
//  multiple threads
//
    {

	TInt r=KErrNone;
	test.Next(_L("Start continuous file Write/Read/Verify operation"));
	RThread t[KMaxNumberThreads];
	TRequestStatus tStat[KMaxNumberThreads];

	TInt i=0;

	TName threadName;
	TRequestStatus kStat=KRequestPending;
	test.Console()->Read(kStat);
	for (i=0;i<KMaxNumberThreads;i++)
		{
		ThreadTestInfo[i].iCycles=0;
		ThreadTestInfo[i].iErrors=0;
		ThreadTestInfo[i].iSizeArrayPos=(i%KMaxSizeArray);
		ThreadTestInfo[i].iErrorInfo=0;
		if (i<(KMaxNumberThreads-1))
			{
			threadName.Format(_L("MakeAndDeleteFiles%d"),i);
	    	r=t[i].Create(threadName,MakeAndDeleteFilesThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)i);
			}
		else
			{
			// Last thread fills/empties disk
			threadName.Format(_L("FillAndEmptyDisk%d"),i);
	    	r=t[i].Create(threadName,FillAndEmptyDiskThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)i);
			}
		if (r!=KErrNone)
   			test.Printf(_L("Error(%d) creating thread(%d)\r\n"),r,i);
	    test(r==KErrNone);
	    t[i].Logon(tStat[i]);
		t[i].Resume();
		}
	CurrentlyFillingDisk=ETrue;
	FillDiskCount=0;

    TInt totalTime = 0;
    TTime cycleTime;
    TTime startTime;
    TTime time;
    startTime.UniversalTime();
    cycleTime.UniversalTime();
    
	TVolumeInfo v;
	r=TheFs.Volume(v,gDriveNumber);
	test(r==KErrNone);
//	TInt initialFreeSpace = I64LOW(v.iFree / 1024);

#ifdef __LIMIT_EXECUTION_TIME__
	RTimer timer;
	timer.CreateLocal();
	TRequestStatus reqStat;
	timer.After(reqStat,60000000); // After 60 secs
#endif

#ifdef REUSE_THREAD
	RTimer displayTimer;
	displayTimer.CreateLocal();
	TRequestStatus displayStat;
	displayTimer.After(displayStat, KNotificationInterval); // after 10 secs
#endif

	TInt ypos=test.Console()->WhereY();
	FOREVER
		{
		User::WaitForAnyRequest();
		if (kStat!=KRequestPending)
			{
			// user requested to end - let threads die
#ifdef REUSE_THREAD				
			gRequestEnd = ETrue;
#endif			
			for (i=0;i<KMaxNumberThreads;i++)
				{
				User::WaitForRequest(tStat[i]);
				}
			break;
			}
#ifdef __LIMIT_EXECUTION_TIME__
		else if (reqStat != KRequestPending)
			{
			// max execution exceeded - wait for threads to die
			TInt totalCycles = 0;
			for (i=0;i<KMaxNumberThreads;i++)
				{
				totalCycles+= ThreadTestInfo[i].iCycles;
				}
			test.Printf(_L("Total cycles = %d\r\n"), totalCycles);
			test.Printf(_L("Waiting for thread death...\r\n"));
			for (i=0;i<KMaxNumberThreads;i++)
				{
				User::WaitForRequest(tStat[i]);
				}
			break;
			}
#endif
		else
			{
			// other notification
			TBool threadFinished=EFalse;
			for (i=0;i<KMaxNumberThreads;i++)
				{
				if (tStat[i]!=KRequestPending && !threadFinished)
					{
					t[i].Close();
					(ThreadTestInfo[i].iCycles)++;
					if (tStat[i]!=KErrNone)
						(ThreadTestInfo[i].iErrors)++;
					threadFinished=ETrue;

					// Launch another thread
					TInt threadNameId=((ThreadTestInfo[i].iCycles)%2)?(i+KMaxNumberThreads):i; // Alternate thread name
					if (i<(KMaxNumberThreads-1))
						{
						threadName.Format(_L("MakeAndDeleteFiles%d"),threadNameId);
	    				r=t[i].Create(threadName,MakeAndDeleteFilesThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)i);
						}
					else
						{
						// Last thread fills/empties disk
						threadName.Format(_L("FillAndEmptyDisk%d"),threadNameId);
	    				r=t[i].Create(threadName,FillAndEmptyDiskThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)i);
						}
					if (r!=KErrNone)
   						test.Printf(_L("Error(%d) creating thread(%d)\r\n"),r,i);
	    			test(r==KErrNone);
	    			t[i].Logon(tStat[i]);
					t[i].Resume();
					}
				test.Console()->SetPos(0,(ypos+i));
   				test.Printf(_L("Thread(%d): % 4d errors in % 4d cycles (%d)\r\n"),i,ThreadTestInfo[i].iErrors,ThreadTestInfo[i].iCycles,ThreadTestInfo[i].iErrorInfo);
				}
				
#ifdef REUSE_THREAD
			if(displayStat != KRequestPending)
				{
				// re-request notification
				displayTimer.After(displayStat, KNotificationInterval);
				}
			else if (!threadFinished)
				{
				test.Printf(_L("Semaphore death"));
				break;
				}
#else 
			if (!threadFinished)
				{
				test.Printf(_L("Semaphore death"));
				break;
				}
#endif					
				
			r=TheFs.Volume(v,gDriveNumber);
			test(r==KErrNone);
			test.Console()->SetPos(0,(ypos+KMaxNumberThreads));
			
			TInt  freeSpace;
			TInt8 freeSpaceUnit;
			TInt  totalSpace;
			TInt8 totalSpaceUnit;
			
			// switch t
			if(v.iFree > KDiskUnitThreshold)
				{
				// display in MB
				freeSpace = I64LOW(v.iFree / (1024 * 1024));
				freeSpaceUnit = 'M';
				}
			else
				{
				// display in KB
				freeSpace = I64LOW(v.iFree/1024);
				freeSpaceUnit = 'K';
				}
			
			if(v.iSize > KDiskUnitThreshold)
				{
				// display in MB
				totalSpace = I64LOW(v.iSize / (1024 * 1024));
				totalSpaceUnit = 'M';
				}
			else
				{
				// display in KB
				totalSpace = I64LOW(v.iSize/1024);
				totalSpaceUnit = 'K';
				}
			
			test.Printf(_L("Free space on disk: %u %cB (of %u %cB)\r\n"), 
						freeSpace, freeSpaceUnit, totalSpace, totalSpaceUnit);

            TTimeIntervalSeconds timeTaken;
            time.UniversalTime();            
            r=time.SecondsFrom(startTime,timeTaken);
            test(r==KErrNone);
            totalTime=timeTaken.Int();

            TInt seconds = totalTime % 60;
            TInt minutes = (totalTime / 60) % 60;
            TInt hours   = (totalTime / 3600) % 24;
            TInt days    = totalTime / (60 * 60 * 24);
            test.Printf(_L("Elapsed Time (%d): %d d %02d:%02d:%02d\r\n"), FillDiskCycle, days, hours, minutes, seconds);
            
            if(CurrentlyFillingDisk)
            	{
            	// work out ETA to full disk
	            r = time.SecondsFrom(cycleTime, timeTaken);
    	        if((r == KErrNone) && (v.iSize > v.iFree))
    	        	{
		            	totalTime = (TInt) ((v.iFree/1024 * (TInt64) timeTaken.Int()) / (v.iSize/1024 - v.iFree/1024));
		    	        seconds = totalTime % 60;
    		    	    minutes = (totalTime / 60) % 60;
        		    	hours   = (totalTime / 3600) % 24;
	            		days    = totalTime / (60 * 60 * 24);
            	
    	        		test.Printf(_L("ETA to full disk: %d d %02d:%02d:%02d\r\n"), days, hours, minutes, seconds);
        	    	}
            	}
            else 
            	{
            	// currently emptying disk, update time metrics
            	cycleTime.UniversalTime();
            	}            	
           	
			}
						
			test.Printf(_L("\n"));
		}
    }
#endif


GLDEF_C void CallTestsL()
//
// Call all tests
//
    {
	TInt r = TheFs.CharToDrive( gSessionPath[0], gDriveNumber );
	test( KErrNone == r );
	
	// select appropriate metrics table
	if(IsFileSystemFAT32(TheFs, gDriveNumber)) 
		{
		TheMetrics = &FAT32Metrics;
		RDebug::Printf("Using FAT32 metrics\r\n");
		}
	else if(IsTestingLFFS())
		{
		TheMetrics = &LffsMetrics;
		RDebug::Printf("Using LFFS metrics\r\n");
		}
	else
		{
		TDriveInfo driveInfo;
		r=TheFs.Drive(driveInfo, gDriveNumber);
		test(r==KErrNone);
		
		if((driveInfo.iType==EMediaNANDFlash) && !(driveInfo.iMediaAtt & KMediaAttWriteProtected))
			{
			TheMetrics = &NandMetrics;
			RDebug::Printf("Using NAND metrics\r\n");
			}
		else 
			{
			TheMetrics = &DefMetrics;
			RDebug::Printf("Using default metrics\r\n");
			}
		}
		
	FillDiskCycle = 1;
#ifdef REUSE_THREAD
	gRequestEnd = EFalse;
#endif
	DoTests();

    }

