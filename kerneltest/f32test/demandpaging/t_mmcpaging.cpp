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
// e32test\mmu\t_mmcpaging.cpp
// Suite of tests specifically to test the demand paging subsystem when
// booted from MMC rather than NAND.
// 002 Read/Write and Page test
// 
//

//! @SYMTestCaseID			KBASE-T_MMCPAGING-0331
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging MMC Paging tests.
//! @SYMTestActions			001 Check that the rom is paged
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#include <e32test.h>
RTest test(_L("T_MMCPAGING"));

#include <e32rom.h>
#include <e32svr.h>
#include <u32hal.h>
#include <f32file.h>
#include <f32dbg.h>
#include <d32locd.h>
#include <hal.h>
#define __TEST_PAGING_MEDIA_DRIVER__
#include "mmcdp.h"



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
TBool CtrlIOSupported=ETrue;


//const TInt KDiskSectorShift = 9;
const TInt KBufSizeInBytes = (32 * 1024);

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



// Finds the 1st MMC drive, or checks the specified one fits requirements  

static TInt FindFsMMCDrive()
	{
	TDriveList driveList;
	TDriveInfo driveInfo;
	TInt r=TheFs.DriveList(driveList);
    test(r == KErrNone);

	TInt drvNum = DriveNumber;
	if (drvNum<0)
		drvNum = 0;
	do
		{
	    if(!driveList[drvNum])
	        continue;   //-- skip unexisting drive

	    test(TheFs.Drive(driveInfo, drvNum) == KErrNone);

		if(driveInfo.iMediaAtt&KMediaAttPageable)
			{
			// Internal MMC ?
			if (driveInfo.iType == EMediaHardDisk && 
				(driveInfo.iDriveAtt & KDriveAttInternal) &&
				(!(driveInfo.iDriveAtt & KDriveAttRemovable)))
				return (drvNum);
			}
		}
	while(DriveNumber<0 && ++drvNum<KMaxDrives);

	return (-1);
	}


//
// Writes to main area for the entire disk and reads back to verify.
// The function is called from TestMmcAccuratcy, which will have also
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
	
	SMmcStats stats;
	TInt reqPageCount=0;	
	TInt reqNormalCount=0;

	
	TInt readSize = KBufSizeInBytes/2;
	TInt writeSize = KBufSizeInBytes/2;

	Buffer.SetLength(2*readSize);

	TPtr8 subBuf1(&Buffer[0],readSize);
	TPtrC8 subBuf2(&Buffer[readSize], readSize);
	
	test.Printf(_L("writeSize = %d\n"), writeSize);

//	TInt64 size = DriveCaps.iSize - (DriveCaps.iSize % readSize);
	
	for(i = 0; i<readSize; i++)
		Buffer[readSize+i] = (char)(i%100);

	// Zero Stats
	if(CtrlIOSupported)
		{
		TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
	 	test(Drive.ControlIO(KMmcGetStats,statsBuf,0) == KErrNone);
		}


	TFileName fileName = _L("?:\\f32-tst\\mmcpage.txt");
	fileName[0] = (TText) ('A'+DriveNumber);


	r = TheFs.MkDirAll(fileName);
	test(r==KErrNone || r== KErrAlreadyExists);
//	fileName += KTempFileName;	
	RFile tempFile;
	r=tempFile.Replace(TheFs,fileName,EFileWrite);
	if (r!=KErrNone)
		test.Printf(_L("Error %d: file '%S' could not be created\n"),r,&fileName);
	test(r==KErrNone);	

	TVolumeInfo volInfo;
	r = TheFs.Volume(volInfo, DriveNumber);
	test (r == KErrNone);

	
	TInt64 size = volInfo.iFree - (volInfo.iFree % readSize);
	TInt maxFileSize = (size > KMaxTInt) ? KMaxTInt : (TInt) size;
	
	test.Printf(_L("Volume size %ld, free %ld maxFileSize %d file '%S'\n"), volInfo.iSize, volInfo.iFree, maxFileSize, &fileName);
	
	while (((totChangeCount<Maxloops) || Forever) && (GlobError==KErrNone))
		{
		
		for(TInt pos=0;
			((pos+writeSize) < maxFileSize) && ((totChangeCount<Maxloops) || Forever) && (GlobError==KErrNone);
			pos+=(TUint)(readSize))
			{
			blockNo=I64LOW(pos / writeSize);
			if (pos % (writeSize) == 0)
				test.Printf(_L("Block %d at %u \r"), blockNo, I64LOW(pos));

			//write the pattern
			r = tempFile.Write(pos,subBuf2);
			if (r != KErrNone)
				test.Printf(_L("Write failed %d"), r);
			test(r==KErrNone);

			//read back and verify
			r = tempFile.Read(pos,subBuf1,readSize);
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
			if (pos % (writeSize) == 0)
				{				

				if ((blockNo%80==0) && (blockNo!=0))
					{
					totChangeCount+=changeCount;
					if(CtrlIOSupported)
						{
						test.Printf(_L("High%4d Avg%2d %d%% CC=%4d \n"), high, (TInt) (tot/cCount), (TInt)(changeCount*100)/cCount, totChangeCount);
						
						TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
						Drive.ControlIO(KMmcGetStats,statsBuf,0);
						test.Printf(_L("PR %d(%d%%) NR %d\n"), stats.iReqPage, (TInt) ((stats.iReqPage*100)/cCount), stats.iReqNormal);

						test(stats.iReqPage>0);
				 		reqPageCount+=stats.iReqPage;			 	
				 		reqNormalCount+=stats.iReqNormal;			 	
						}

					high=0;
					
					fullTot+=tot;
					tot=0;
					
					fullcCount+=cCount;
					cCount=0;
					changeCount=0;
					}
						
				}
			test(r==KErrNone);
			}
		if(CtrlIOSupported)
			{
			test.Printf(_L("Totals: Avg %2d %d%% CC=%4d \n"), fullTot/fullcCount, (TInt)(totChangeCount*100)/fullcCount, totChangeCount);
			test.Printf(_L("PR %d(%d%%) NR %d\n"), reqPageCount,(TInt) (reqPageCount*100/fullcCount), reqNormalCount );
			}

		// If totChangeCount does not change, mmc maybe busy waiting.
		test(totChangeCount>0);
		}


	tempFile.Close();
	r = TheFs.Delete(fileName);
	test (r == KErrNone);
	
	if (GlobError!=KErrNone)
		{
		test.Printf(_L("\nPageing failed with %x\n"), GlobError);
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
//	RTest test(_L("RepeatedPagingThread"));
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

void TestMmcAccuratcy()
	{
	RThread thisThread;
	const TInt KNoThreads=10;
	TInt i;
	test.Printf(_L("Reset stats\n"));

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

		for (i=0; i<KNoThreads; i++)
			{
			test(repeatedPagingThread[i].Create(_L(""),RepeatedPagingThread,KDefaultStackSize,NULL,(TAny*) ETrue)==KErrNone);
			repeatedPagingThread[i].Logon(stat[i]);
			test(stat[i]==KRequestPending);	
			repeatedPagingThread[i].Resume();
			}
		// Start repeated paging.
		thisThread.SetPriority(EPriorityMore);
		Testing=ETrue;
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
		}
		else
			test.Next(_L("Read/Write test - Skipped!"));

	}
	

// ************************************************************************************
	
	
/*
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
	TBool flush;
	while (Testing)
		{
		// Wait on semaphore
		PageSemaphore.Wait();
		flush = (PagesBeingPaged==0);
		PagesBeingPaged++;
		addr=start+((TInt64(Random())*TInt64(size))>>32);	
		PageDoneSemaphore.Signal();
		if (flush)
			UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
		READ(addr);
		PageMutex.Wait();
		PagesBeingPaged--;
		PageMutex.Signal();
		}
	return 0;
	}
*/

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
				test.Printf(_L("\nUsage:  t_mmcpaging <driveletter> [rwsoak <cc>] [defer <c>]\n'-' indicated infinity.\n\n"));
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
	test.Printf(_L("PR: Paging requests\n"));
	test.Printf(_L("NR: Normal requests\n\n"));

	
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
			
		DriveNumber = FindFsMMCDrive();	
		
		if(DriveNumber<0)
			test.Printf(_L("MMC Flash not found - test skipped!\r\n"));
		else
			{
			RFile file;
			TBuf<256> fileName;	
			fileName.Append((TChar)('A'+DriveNumber));
			fileName+=_L(":\\f32-tst\\");
			TInt r=TheFs.MkDirAll(fileName);
			test(r==KErrNone || r== KErrAlreadyExists);
			fileName += _L("redglare.txt");
			r=file.Replace(TheFs,fileName,EFileWrite);
			if (r!=KErrNone)
				test.Printf(_L("Error %d: file '%S' could not be created\n"),r,&fileName);
			test(r==KErrNone);
			r=file.Write(_L8("The red glare of an ancient sun reflecting on the leaden surface of a primeval soup of decomposing matter"));
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
			test.Printf(_L("Found drive: %c (MMC drive %d)\r\n"), DriveNumber+'A',locDriveNumber);
			file.Close();
			
			TDriveInfo driveInfo;
			test(TheFs.Drive(driveInfo, DriveNumber) == KErrNone);

			// Connect to device driver
			TBool changeFlag = EFalse;
			r = Drive.Connect(locDriveNumber,changeFlag);
			TPckg<TLocalDriveCapsV4>	capsPack(DriveCaps);
			Drive.Caps(capsPack);
			test(r == KErrNone);

			SMmcStats stats;
			TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
			r = Drive.ControlIO(KMmcGetStats,statsBuf,0);


			if (r!=KErrNone)
				{
				test.Printf(_L("LocalDrive does not support testing IO Requests\n"));
				CtrlIOSupported=EFalse;
				}
			test.Printf(_L("LocalDrive Connected\n"));
			//
			// Run tests	
			//
			TestMmcAccuratcy();
			// 
			// Free device and end test program
			//
			Drive.Disconnect();
			}	
		}
		
	test.End();
	return 0;
	}

