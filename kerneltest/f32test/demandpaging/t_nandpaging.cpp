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
// e32test\mmu\t_nandpaging.cpp
// Suite of tests specifically to test the demand paging subsystem when
// booted from NAND.
// 002 Read/Write and Page test
// 003 Defering test
// 
//

//! @SYMTestCaseID			KBASE-T_NANDPAGING-0332
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging Nand Paging tests.
//! @SYMTestActions			001 Check that the rom is paged
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#define __E32TEST_EXTENSION__
#include <e32test.h>
RTest test(_L("T_NANDPAGING"));

#include <e32rom.h>
#include <e32svr.h>
#include <u32hal.h>
#include <f32file.h>
#include <f32dbg.h>
#include "testdefs.h"
#include <hal.h>
#include "nfe.h"


TInt DriveNumber=-1;   // Parameter - Which drive?  -1 = autodetect.
TInt locDriveNumber;

TInt MaxDeferLoops=40; // Parameter - Defer test, for how long?
TInt Maxloops=400;	   // Parameter - RW Soak, for how long?
TBool Forever=EFalse;  // Parameter - RW Soak forever?

TBool Testing=ETrue;	// Used to communicate when testing has finished between threads.

RFs TheFs;
TBusLocalDrive Drive;
TLocalDriveCapsV4 DriveCaps;

TInt PagedTrashCount=0; // Incremented by threads, is used to detect preemption.
TInt GlobError=KErrNone; // To communicate an error between threads.
TBool CtrlIoCollectGarbageSupported = ETrue;
TBool CtrlIoGetDeferStatsSupported = ETrue;


const TInt KDiskSectorShift=9;
const TInt KBufSizeInSectors=8;
const TInt KBufSizeInBytes=(KBufSizeInSectors<<KDiskSectorShift)*40;

LOCAL_D TBuf8<KBufSizeInBytes> Buffer;



// Three functions for the garbage test.
// CreateFile creates a file, and sets up the buffer for WriteNumber.
// After the code has finished writing numbers to the start,
// CloseAndDestroy cleans up.

void CreateFile(RFile &aFile,const TDesC& aFileName)
	{
	TBuf<256> fileName;	
	fileName.Append((TChar)('A'+DriveNumber));
	fileName+=_L(":\\f32-tst\\");
	TInt r=TheFs.MkDirAll(fileName);
	test(r==KErrNone || r== KErrAlreadyExists);

	fileName += aFileName;	

	r=aFile.Replace(TheFs,fileName,EFileWrite);
	if (r!=KErrNone)
		test.Printf(_L("Error %d: file '%S' could not be created\n"),r,&fileName);
	test(r==KErrNone);	
	Buffer.SetLength(4);
	}
	
void CloseAndDestroy(RFile &aFile)
	{
	TBuf<256> fileName;	
	aFile.FullName(fileName);
	aFile.Close();
	TheFs.Delete(fileName);
	}

TInt WriteNumber(RFile &aFile)
	{
	TInt r;
	Buffer[0]++;
	r = aFile.Write(0,Buffer);
	if (r==KErrNone)
		return aFile.Flush();
	else
		return r; 
	}



// The r/w soaktest leaves the drive in a mess.
// Formatting is needed afterwards.

void silentFormat(TInt driveNo) 
	{    
    TBuf<4> driveBuf=_L("?:\\");
    RFormat format;
    TInt    count;
    
	driveBuf[0] = (TText)(driveNo + 'A');
    
    TInt r = format.Open(TheFs, driveBuf, EHighDensity, count);
    test(r == KErrNone);
    
    while(count) 
		{
        r=format.Next(count);
        test(r == KErrNone);
		}
    
    format.Close();
	}

// Finds the 1st r/w NAND drive, or checks the specified one fits requirements  

static TInt FindFsNANDDrive()
	{
	TDriveList driveList;
	TDriveInfo driveInfo;
	TInt r=TheFs.DriveList(driveList);
    test(r == KErrNone);
	
	for (TInt drvNum= (DriveNumber<0)?0:DriveNumber; drvNum<KMaxDrives; ++drvNum)
		{
	    if(!driveList[drvNum])
	        continue;   //-- skip unexisting drive

	    test(TheFs.Drive(driveInfo, drvNum) == KErrNone);

		if ((driveInfo.iMediaAtt&KMediaAttPageable) &&
			(driveInfo.iType == EMediaNANDFlash) && 
			(driveInfo.iDriveAtt & KDriveAttInternal))
			{
			TBool readOnly = driveInfo.iMediaAtt & KMediaAttWriteProtected;		// skip ROFS partitions
			if(!readOnly)
				{
				if ((drvNum==DriveNumber) || (DriveNumber<0))		// only test if running on this drive
					{
					return (drvNum);
					}
				}
			}
		}
	return (-1);
	}


//
// Writes to main area for the entire disk and reads back to verify.
// The function is called from TestNandAccuratcy, which will have also
// started the background RepeatedPagingThread
//
void testWriteMain()
	{
	TInt i;
	TInt r;
	TInt changeCount=0;
	TInt totChangeCount=0;
	TInt cCount=0;
	TInt fullcCount=0;
	TInt oldPagedTrashCount=0;
	TInt delta=0;
	TInt high=0;
	TInt tot=0;
	TInt fullTot=0;
	TInt blockNo;

	// read size is 64K
	TInt readSize = (64*1024);	
	TInt64 size = DriveCaps.iSize - (DriveCaps.iSize % readSize);
	
	// print position every 128K
	TInt64 printBlockPos = 128 * 1024;
	test (size > printBlockPos);

	// check for paging activity every 1MB
	TInt64 checkChangePos = 1024*1024;
	while (checkChangePos > size)
		checkChangePos>>= 1;

	
	SDeferStats stats;
	TInt pageGarbageCount=0;
	TInt pageOtherCount=0;	
	TInt normalGarbageCount=0;
	TInt normalOtherCount=0;
	
	
	Buffer.SetLength(2*readSize);

	TPtr8 subBuf1(&Buffer[0],readSize);
	TPtrC8 subBuf2(&Buffer[readSize], readSize);
	
	test.Printf(_L("Page size = %d\n"), DriveCaps.iNumBytesMain);
	test.Printf(_L("Erase block size = %d\n"), DriveCaps.iEraseBlockSize);
	test.Printf(_L("Media size (rounded down) = %ld\n"), size);

	for(i = 0; i<readSize; i++)
		Buffer[readSize+i] = (char)(i%100);

	// Zero Stats
	if(CtrlIoGetDeferStatsSupported)
		{
		TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
 		test(Drive.ControlIO(KNandGetDeferStats,statsBuf,0)==KErrNone);
		}


	while (((totChangeCount<Maxloops) || Forever) && (GlobError==KErrNone))
		{
		for(TInt64 pos=0;
			(pos<size) && ((totChangeCount<Maxloops) || Forever) && (GlobError==KErrNone);
			pos+=(TUint)(readSize))
			{
			blockNo=I64LOW(pos / DriveCaps.iEraseBlockSize);
			if ((pos % printBlockPos) == 0)
				test.Printf(_L("Block %d at pos %lu \r"), blockNo, pos);

			//write the pattern
			r = Drive.Write(pos,subBuf2);
			test(r==KErrNone);

			//read back and verify
			r = Drive.Read(pos,readSize,subBuf1);
			test(r==KErrNone);

			for(i=0;i<readSize;i++)
				if(Buffer[i]!=Buffer[readSize+i])
					{
					r = KErrCorrupt;
					break;
					}
			delta = PagedTrashCount-oldPagedTrashCount;
			cCount++;
			if (delta)
				{	
				if (delta>high)
					high=delta;
				tot+=delta;
				
				oldPagedTrashCount=PagedTrashCount;
				changeCount++;
				}

			if ((pos > 0) && (pos % checkChangePos) == 0)
				{
				totChangeCount+=changeCount;
				if(CtrlIoGetDeferStatsSupported)
					{
					test.Printf(_L("\nHigh%4d Avg%2d %d%% CC=%4d \n"), high, (TInt) (tot/cCount), (TInt)(changeCount*100)/cCount, totChangeCount);
				
					TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
					Drive.ControlIO(KNandGetDeferStats,statsBuf,0);
					test.Printf(_L("PG %d PO %d(%d%%) NG %d NO %d\n"),stats.iPageGarbage,  stats.iPageOther, (TInt) ((stats.iPageOther*100)/cCount), stats.iNormalGarbage,  stats.iNormalOther);

				 	pageGarbageCount+=stats.iPageGarbage; 
				 	pageOtherCount+=stats.iPageOther;			 	
				 	normalGarbageCount+=stats.iNormalGarbage; 
				 	normalOtherCount+=stats.iNormalOther;			 	
					}

				high=0;
				
				fullTot+=tot;
				tot=0;
				
				fullcCount+=cCount;
				cCount=0;
				changeCount=0;
				}

			test(r==KErrNone);
			}	// for loop

		if (CtrlIoGetDeferStatsSupported)
			{
			test.Printf(_L("\nTotals: Avg %2d %d%% CC=%4d \n"), fullTot/fullcCount, (TInt)(totChangeCount*100)/fullcCount, totChangeCount);
			test.Printf(_L("PG %d PO %d(%d%%) NG %d NO %d\n"),pageGarbageCount,  pageOtherCount,(TInt) (pageOtherCount*100/fullcCount), normalGarbageCount,  normalOtherCount );
			test(pageOtherCount > 0);	// Ensure at least one paging conflict occurred during the test.
			}

		// If totChangeCount does not change, nand maybe busy waiting.
		test(totChangeCount>0);
		}	// while ()

	if (GlobError!=KErrNone)
		{
		test.Printf(_L("\nPaging failed with %x\n"), GlobError);
		test(0);
		}
	else
		test.Printf(_L("\ndone\n"));
	}


TUint8 ReadByte(volatile TUint8* aPtr)
	{
	return *aPtr;
	}

#define READ(a) ReadByte((volatile TUint8*)(a))

TUint32 RandomNo =0;

TUint32 Random()
	{
	RandomNo = RandomNo*69069+1;
	return RandomNo;
	}


// Many instances of this run while testWriteMain runs,
// to cause random background paging.

LOCAL_C TInt RepeatedPagingThread(TAny* aUseTb)
	{
	TBool trashBurst = EFalse;
	// This makes the paging system continually page stuff.
	// get info about a paged ROM...
	
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	TUint8* start = (TUint8*)romHeader+romHeader->iPageableRomStart;
	TUint size = romHeader->iPageableRomSize;
	TInt pageSize = 0;
	PagedTrashCount=1;

	UserSvr::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0);
	RandomNo=123;
	PagedTrashCount++;

	while (Testing)
		{
		TInt r=UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
		if (Random() & 1)
			User::AfterHighRes(500+Random() & 2047);

		if (r<0)
			{
			GlobError=r;
			PagedTrashCount=99;
			return (KErrNone);
			}
		if (trashBurst)
			{
			if ((Random() & 0xf) == 0xf)
				trashBurst=EFalse;
			PagedTrashCount++;
			}
		else 
			{
			
			for(TInt i=size/(pageSize); (i>0) && !trashBurst; --i)
				{
				READ(start+((TInt64(Random())*TInt64(size))>>32));
				if ((RandomNo & 0x3f) == 0x3f)
					{
					trashBurst= (TBool) aUseTb;
					}
				PagedTrashCount++;
				if (RandomNo & 1)
					User::AfterHighRes(500+Random() & 2047);
				}
			}
	
		}
	return(KErrNone);
	}
	

// This starts up multiple instances of repeatedPagingThread, and runs testWriteMain.
// After its done, it calls format, to clean up the drive.

void TestNandAccuratcy()
	{
	RThread thisThread;
	const TInt KNoThreads=10;
	TInt i;
	test.Printf(_L("Reset concurrency stats\n"));

	i=UserSvr::HalFunction(EHalGroupMedia,EMediaHalResetConcurrencyInfo,(TAny*)locDriveNumber,(TAny*)EMediaPagingStatsRom);
	test(i==KErrNone || i==KErrNotSupported);
	if(i==KErrNotSupported)
		test.Printf(_L("Concurrency stats not supported on this build\n"));
	i=UserSvr::HalFunction(EHalGroupMedia,EMediaHalResetPagingBenchmark,(TAny*)locDriveNumber,(TAny*)EMediaPagingStatsRom);
	test(i==KErrNone || i==KErrNotSupported);
	if(i==KErrNotSupported)
		test.Printf(_L("Benchmark stats not supported on this build\n"));

	if (Maxloops>0)
		{
		TRequestStatus stat[KNoThreads];
		// Start Read Test
		RThread repeatedPagingThread[KNoThreads];
		
		test.Next(_L("Read/Write and Page test"));

		Testing=ETrue;
		for (i=0; i<KNoThreads; i++)
			{
			
			test(repeatedPagingThread[i].Create(_L(""),RepeatedPagingThread,KDefaultStackSize,NULL,(TAny*) ETrue)==KErrNone);
			repeatedPagingThread[i].Logon(stat[i]);
			test(stat[i]==KRequestPending);	
			repeatedPagingThread[i].Resume();
			}
		// Start repeated paging.
		thisThread.SetPriority(EPriorityMore);
		testWriteMain();
		Testing = 0;
		thisThread.SetPriority(EPriorityNormal);
		for (i=0; i<KNoThreads; i++)
	 		User::WaitForRequest(stat[i]);

		test.Printf(_L("Collect concurrency stats\n"));
		SMediaROMPagingConcurrencyInfo info;
		SPagingBenchmarkInfo infoBench;
		i=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetROMConcurrencyInfo,(TAny*)locDriveNumber,&info);
		test(i==KErrNone || i==KErrNotSupported);
		TInt r=UserSvr::HalFunction(EHalGroupMedia,EMediaHalGetROMPagingBenchmark,(TAny*)locDriveNumber,&infoBench);
		test(r==KErrNone || r==KErrNotSupported);
		if(i==KErrNone)
			{
			test.Printf(_L("Media concurrency stats:\n\n"));
			test.Printf(_L("The total number of page in requests issued whilst processing other page in requests: %d\n"),info.iTotalConcurrentReqs);
			test.Printf(_L("The total number of page in requests issued with at least one queue not empty: %d\n"),info.iTotalReqIssuedNonEmptyQ);
			test.Printf(_L("The maximum number of pending page in requests in the main queue any time during this session: %d\n"),info.iMaxReqsInPending);
			test.Printf(_L("The maximum number of pending page in requests in the deferred queue any time during this session: %d\n"),info.iMaxReqsInDeferred);
			test.Printf(_L("The total number of page in requests first-time deferred during this session: %d\n"),info.iTotalFirstTimeDeferrals);
			test.Printf(_L("The total number of page in requests re-deferred during this session: %d\n"),info.iTotalReDeferrals);
			test.Printf(_L("The maximum number of deferrals of any single page in request during this session: %d\n"),info.iMaxDeferrals);
			test.Printf(_L("The total number of times the main queue was emptied when completing an asynchronous request during this session: %d\n"),info.iTotalSynchEmptiedMainQ);
			test.Printf(_L("The total number of page in requests serviced from main queue when completing an asynchronous request: %d\n"),info.iTotalSynchServicedFromMainQ);
			test.Printf(_L("The total number of page in requests deferred after being picked out of main queue when completing an asynchronous request: %d\n"),info.iTotalSynchDeferredFromMainQ);
			test.Printf(_L("The total number of times the page in DFC run with an empty main queue during this session: %d\n"),info.iTotalRunDry);
			test.Printf(_L("The total number of dry runs of paging DFC avoided during this session: %d\n"),info.iTotalDryRunsAvoided);
			}

		if(r==KErrNone)
			{
			TInt freq = 0;
			r = HAL::Get(HAL::EFastCounterFrequency, freq);
			if (r==KErrNone)
				{
				TReal mult = 1000000.0 / freq;
				TReal min = 0.0;
				TReal max = 0.0;
				TReal avg = 0.0;
				if (infoBench.iCount != 0)
					{
					min = infoBench.iMinTime * mult;
					max = infoBench.iMaxTime * mult;
					avg = (infoBench.iTotalTime * mult) / infoBench.iCount;
					}
				test.Printf(_L("Media benchmarks:\n\n"));
				test.Printf(_L("The total number of page in requests issued: %d\n"),infoBench.iCount);
				test.Printf(_L("The average latency of any page in request in the Media subsystem: %9.1f(us)\n"),avg);
				test.Printf(_L("The maximum latency of any page in request in the Media subsystem: %9.1f(us)\n"),max);
				test.Printf(_L("The minimum latency of any page in request in the Media subsystem: %9.1f(us)\n"),min);
				}
			}

		test.Printf(_L("Formatting...\n"));
		silentFormat(DriveNumber);
		}
		else
			test.Next(_L("Read/Write test - Skipped!"));

	}
	

// ************************************************************************************
	
	
// This code causes a flush
// It is done in a second thread to see if you really do get just
// one deferral, with the other page requests just waiting in line.
// (Paging is not re-entrant)

TInt PagesBeingPaged=0;
RMutex PageMutex;
RSemaphore PageSemaphore;
RSemaphore PageDoneSemaphore;
 
LOCAL_C TInt CausePage(TAny*)
	{	
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	TUint8* start = (TUint8*)romHeader+romHeader->iPageableRomStart;
	TUint size = romHeader->iPageableRomSize;
	TUint8* addr=NULL;
	while (Testing)
		{
			PageSemaphore.Wait(); // wait for main thread to want paging.
			addr=start+((TInt64(Random())*TInt64(size))>>32);	
			PageDoneSemaphore.Signal(); // Acknowledge request.

			PageMutex.Wait();
			TBool flush = (PagesBeingPaged==0);	// Ensure only one thread is flushing the cache at a time.
			PagesBeingPaged++;
			PageMutex.Signal();

			if (flush)
				UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
			READ(addr);

			PageMutex.Wait();
			PagesBeingPaged--;
			PageMutex.Signal();
		}
	return 0;
	}


// TestDefered causes garbage collection, and then triggers paging to happen, which should be defered.
// One would only expect one defered request, as the paging system is not reentrant, but this is checked.

void TestDefered()
	{
	if (MaxDeferLoops==0)
		{
		test.Next(_L("Defering test - Skipped!"));
		return;
		}
		
	// If the NFE test media driver extension is present, ALL the drive is encrypted;
	// this means that there will be very few free blocks in the free block reservoir: this effectively 
	// disables background garbage collection and all block erasing needs to happen on the fly...
	TNfeDeviceInfo nfeDeviceinfo;
	TPtr8 nfeDeviceInfoBuf((TUint8*) &nfeDeviceinfo, sizeof(nfeDeviceinfo));
	nfeDeviceInfoBuf.FillZ();
	TInt r = Drive.QueryDevice((RLocalDrive::TQueryDevice) EQueryNfeDeviceInfo, nfeDeviceInfoBuf);
/*
	if (r == KErrNone)
		{
		test.Printf(_L("NFE device detected, aborting garbage collection test for now\n"));
		return;
		}
*/
	// Create some free blocks by creating a huge file and then deleting it....
	if (r == KErrNone)
		{
		test.Printf(_L("NFE device detected\n"));
		RFile file;

		TBuf<256> tempFileName = _L("?:\\f32-tst\\");
		tempFileName[0] = 'A'+DriveNumber;

		r = TheFs.MkDirAll(tempFileName);
		test(r==KErrNone || r== KErrAlreadyExists);

		tempFileName+= _L("TEMP.TXT");

		r = file.Replace(TheFs, tempFileName, EFileWrite);
		test_KErrNone(r);
		
		for (TInt fileSize = KMaxTInt; fileSize > 0; fileSize >>= 1)
			{
			r = file.SetSize(fileSize);
			test.Printf(_L("Setting file size to %d, r %d\n"), fileSize, r);
			if (r == KErrNone)
				break;
			}
		file.Close();
		r = TheFs.Delete(tempFileName);
		test_KErrNone(r);
		}



	TInt timeout;
	TInt writesNeeded=100;
	RFile tempFile;
	TInt i;
	TInt ii;
	TInt runs=0;

	SDeferStats stats;
	TInt pageGarbageCount=0;
	TInt pageOtherCount=0;	
	TInt normalGarbageCount=0;
	TInt normalOtherCount=0;


	// Set up thread sync
	test(PageMutex.CreateLocal()==KErrNone);
	test(PageSemaphore.CreateLocal(0)==KErrNone);
	test(PageDoneSemaphore.CreateLocal(0)==KErrNone);

		

	const TInt KMaxPageThreads = 2;
	UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
	// Set up threads
	RThread pageThread[KMaxPageThreads];
	TRequestStatus stat[KMaxPageThreads];
	Testing=ETrue;
	for (i=0; i<KMaxPageThreads; i++)
		{
		test(pageThread[i].Create(_L(""),CausePage,KDefaultStackSize,NULL,NULL)==KErrNone);
		pageThread[i].Logon(stat[i]);
		test(stat[i]==KRequestPending);
		pageThread[i].SetPriority(EPriorityMore);
		pageThread[i].Resume();
		}
		
		
	test.Next(_L("Defering test"));

	// clear counters
	TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
	test(Drive.ControlIO(KNandGetDeferStats,statsBuf,0)==KErrNone);
	
	CreateFile(tempFile,_L("nandpage.txt"));
		
	 	
	for (ii=0; ii<MaxDeferLoops; ii++)  // Repeat the test, 'MaxDeferLoops' number of times.  This can be set on cammand line.
		{
		writesNeeded=100;
		timeout=20;
		do  // while ((pageGarbageCount==0) && (timeout>0));
			// ie, while garbage collection hasn't happened, or timed out
			{
			timeout--;
			pageGarbageCount=0;
			pageOtherCount=0;
			normalGarbageCount=0;
			normalOtherCount=0;
			
			// Give somethng for garbage collector to collect	
			for (i=0; i<writesNeeded; i++)
		 		test(WriteNumber(tempFile)==KErrNone);
			 
			// Force Collection.  (Normally only happens in Idle) 		
		 	r = Drive.ControlIO(KNandCollectGarbage,NULL,NULL);
		 	test(r==KErrNone);
			 	
		 	// Since garbage Colleciton should be going now, watch it, until its finished. 
			do
		 		{
				runs = PagesBeingPaged;
				for (i=runs; i<KMaxPageThreads; i++)
		 			PageSemaphore.Signal(); // Trigger Paging.

				for (i=runs; i<KMaxPageThreads; i++)
		 			PageDoneSemaphore.Wait();
					
				TInt tries = 10;
				do { // If we get zero hits, maybe the page hasnt hit yet.
					tries--;
					User::AfterHighRes(1000+Random() & 2047);  // Throw some uncertainly into things
				
					TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
			 		r = Drive.ControlIO(KNandGetDeferStats,statsBuf,0);
					test (r == KErrNone);
			 		pageGarbageCount+=stats.iPageGarbage; 
			 		pageOtherCount+=stats.iPageOther;			 	
			 		normalGarbageCount+=stats.iNormalGarbage; 
			 		normalOtherCount+=stats.iNormalOther;
					} while ((pageGarbageCount==0) && (tries>0)); // If we get zero hits, maybe the page hasnt hit yet
		 		}
		 	while (stats.iPageGarbage>0); // Keep going until collection seems to have finished.
		 	
		 	// The paging system is not reentrant, so should never get more then one.
		 	test(stats.iPageGarbage<2);
		 	
			test.Printf(_L("%d: PG %d PO %d NG %d NO %d\n"),ii,pageGarbageCount,  pageOtherCount, normalGarbageCount,  normalOtherCount );
			// if no collection, probebly didnt write enough to trigger it, so up the quantity.
			if (pageGarbageCount==0)
				{		
				writesNeeded+=writesNeeded/2;
				test.Printf(_L("Writes needed = %d\n"),writesNeeded);
				}
				
			}
		while ((pageGarbageCount==0) && (timeout>0));
		test(timeout>0);
			
		} // end for MaxDeferLoops.

	// Clean up. . . . .

	Testing=EFalse;  // Setting this causes the CausePage threads to exit.

	// Wait for threads to exit, signaling the semaphore in case they where waiting on it.
	for (i=0; i<KMaxPageThreads; i++)
		PageSemaphore.Signal();
	for (i=0; i<KMaxPageThreads; i++)
		User::WaitForRequest(stat[i]);

	PageMutex.Close();	
	PageSemaphore.Close();
	PageDoneSemaphore.Close();
	CloseAndDestroy(tempFile);
	}


// ************************************************************************************

//
// The gubbins that starts all the tests
//
// ParseCommandLine reads the arguments and sets globals accordingly.
//

void ParseCommandLine()
	{
	TBuf<32> args;
	User::CommandLine(args);
	TLex lex(args);
	
	FOREVER
		{
		
		TPtrC token=lex.NextToken();
		if(token.Length()!=0)
			{
			if ((token.Length()==2) && (token[1]==':'))
				DriveNumber=User::UpperCase(token[0])-'A';
			else if (token.Length()==1)
				{
				TChar driveLetter = User::UpperCase(token[0]); 
				if ((driveLetter>='A') && (driveLetter<='Z'))
					DriveNumber=driveLetter - (TChar) 'A';
				else 
					test.Printf(_L("Unknown argument '%S' was ignored.\n"), &token);
				}
			else if ((token==_L("help")) || (token==_L("-h")) || (token==_L("-?")))
				{
				test.Printf(_L("\nUsage:  t_nandpaging <driveletter> [rwsoak <cc>] [defer <c>]\n'-' indicated infinity.\n\n"));
				test.Getch();
				Maxloops=0;
				}
			else if (token==_L("rwsoak"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt v;

				if (val==_L("-"))
					Forever=ETrue;
				else
					if (lexv.Val(v)==KErrNone)
						Maxloops=v;
					else
						test.Printf(_L("Bad value for rwsoak '%S' was ignored.\n"), &val);
				}
			else if (token==_L("defer"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt v;

				if (val==_L("-"))
					MaxDeferLoops=KMaxTInt;
				else
					if (lexv.Val(v)==KErrNone)
						MaxDeferLoops=v;
					else
						test.Printf(_L("Bad value for defer '%S' was ignored.\n"), &val);
				}	
			else
				test.Printf(_L("Unknown argument '%S' was ignored.\n"), &token);
			}
		else
			break;
		
		}
	}

//
// E32Main
//

TInt E32Main()
	{
	TInt r;
	test.Title();

	test.Printf(_L("key\n---\n"));	
	test.Printf(_L("PG: Paging requests defered due to Garbage\n"));
	test.Printf(_L("PO: Paging requests defered due to other operations\n"));
	test.Printf(_L("NG: Normal requests defered due to Garbage\n"));
	test.Printf(_L("NO: Normal requests defered due to other operations\n\n"));

	
	test.Start(_L("Check that the rom is paged"));
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();

	if (romHeader->iPageableRomStart==NULL)
		test.Printf(_L("Test ROM is not paged - test skipped!\r\n"));
	else
		{
		ParseCommandLine();	
		test(TheFs.Connect()==KErrNone);

		r=UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
		if(r<0)
			{
			test.Printf(_L("DemandPagingFlushPages Error = %d\n"),r);
			test(0);
			}

		DriveNumber = FindFsNANDDrive();	
		
		if(DriveNumber<0)
			test.Printf(_L("NAND Flash not found - test skipped!\r\n"));
		else
			{
			RFile file;
			TBuf<256> fileName;	
			fileName.Append((TChar)('A'+DriveNumber));
			fileName+=_L(":\\f32-tst\\");
			TInt r=TheFs.MkDirAll(fileName);
			test(r==KErrNone || r== KErrAlreadyExists);
			fileName += _L("annoyingflies.txt");
			r=file.Replace(TheFs,fileName,EFileWrite);
			if (r!=KErrNone)
				test.Printf(_L("Error %d: file '%S' could not be created\n"),r,&fileName);
			test(r==KErrNone);
			r=file.Write(_L8("Flies as big as sparrows indoletly buzzing in the warm air, heavy with the stench of rotting carcasses"));
			if (r!=KErrNone)
				test.Printf(_L("Error %d: could not write to file\n"),r);
			test(r==KErrNone);

			test(file.Flush() == KErrNone);

			SBlockMapInfo info;
			TInt64 start=0;
			r=file.BlockMap(info,start, -1,ETestDebug);
			if (r!=KErrNone && r!=KErrCompletion)
				test.Printf(_L("Error %d: could not obtain block map\n"),r);
			test(r==KErrNone || r==KErrCompletion);
			locDriveNumber=info.iLocalDriveNumber;
			test.Printf(_L("Found drive: %c (NAND drive %d)\r\n"), DriveNumber+'A',locDriveNumber);
			file.Close();
			
			// Connect to device driver
			TBool changeFlag = EFalse;
			r = Drive.Connect(locDriveNumber,changeFlag);
			TPckg<TLocalDriveCapsV4>	capsPack(DriveCaps);
			Drive.Caps(capsPack);
			test(r == KErrNone);

			r = Drive.ControlIO(KNandCollectGarbage,NULL,NULL);
			if (r!=KErrNone)
				{
				test.Printf(_L("LocalDrive does not support the KNandCollectGarbage ControlIO request\n"));
				CtrlIoCollectGarbageSupported = EFalse;
				}

			SDeferStats stats;
			TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
	 		r = Drive.ControlIO(KNandGetDeferStats,statsBuf,0);
			if (r == KErrNone)
				{
				if (stats.iSynchronousMediaDriver)
					{
					test.Printf(_L("Media drive is synchronous - test skipped!\r\n"));
					test.End();
					return 0;
					}
				}
			else
				{
				test.Printf(_L("LocalDrive does not support the KNandGetDeferStats ControlIO request\n"));
				CtrlIoGetDeferStatsSupported = EFalse;
				}


			test.Printf(_L("LocalDrive Connected\n"));
			//
			// Run tests	
			//
			TestNandAccuratcy();
			if(CtrlIoCollectGarbageSupported && CtrlIoGetDeferStatsSupported)
				TestDefered();
			// 
			// Free device and end test program
			//
			Drive.Disconnect();
			}	
		}
		
	test.End();
	return 0;
	}


