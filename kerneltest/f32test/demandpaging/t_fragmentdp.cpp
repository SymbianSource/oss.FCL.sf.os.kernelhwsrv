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
// f32test\demandpaging\t_fragment.cpp
// This test exercises the fragmentation of write requests carried out 
// by the Local Media subsystem, when the request is for a partition 
// driven by a Media driver that supports paging.
// 002 Check if LFFS drive (Mount LFFS if required)
// 003 Testing Fragmentation of writes to writable drives in paging media
// 004 Testing concurrent Fragmentation of writes on the same media
// 005 Check Disk
// 
//

//! @SYMTestCaseID			KBASE-T_FRAGMENTDP-0333
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging Page cache fragmentation tests.
//! @SYMTestActions			001 Starting tests...
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#include <f32file.h>
#include <d32locd.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"
#include <u32hal.h>
#include <e32rom.h>
#include <f32dbg.h>
#include "testdefs.h"

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

const TInt KMuliplySize=10;
const TInt KFileSizeInBytes=302498;

LOCAL_D TBuf8<KMuliplySize*KFileSizeInBytes> Buffer;
LOCAL_D RSemaphore WriteSemaphore;

GLDEF_D RTest test(_L("T_FRAGMENTDP"));

void DoTestF(TInt aDrvNum, TBool aNand);	// may want to do something weird on NAND later (e.g. trigger Garbage Collection)
void DoTestC(TInt aDrvNum, TInt aNotherDrvNum, TBool aNand);	// may want to do something weird on NAND later (e.g. trigger Garbage Collection)
TInt CreateEmptyFile(RFs& aFs, const TDesC& aFileName, TUint aFileSize);
TInt GetLocDrvNumber(TInt aDrvNo);

/*
  This plain looking test exercises the fragmentation of write requests carried out by the Local
  Media subsystem, when the request is for a partition driven by a Media driver that supports
  paging.
  It indirectly tests that the ELOCD fragmentation and EKERN locking mechanisms work as specified 
  to prevent deadlocks. It also causes an awful lot of paging activity.
*/

LOCAL_C TBool TestSimpleFragmentation()
//
// Find ROM address of file and write from it to another file in writable partition in the same media as the backing store for ROM
//

	{
	TDriveInfo driveInfo;
	TBool tested=EFalse;

	TFileName path;
	TInt r=TheFs.SessionPath(path);
	test(r==KErrNone);
	TInt drv;
	r=RFs::CharToDrive(path[0],drv);
	test(r==KErrNone);

    test(TheFs.Drive(driveInfo, drv) == KErrNone);

	//-- select a suitable drive for the testing. It shall be a writable drive on a media that services paging
	if(driveInfo.iMediaAtt&KMediaAttPageable)
		{
		TBool readOnly = driveInfo.iMediaAtt & KMediaAttWriteProtected;		// skip ROFS partitions
		if(!readOnly)
			{
			DoTestF(drv, (driveInfo.iType==EMediaNANDFlash)?(TBool)ETrue:(TBool)EFalse);
			tested=ETrue;
			}
		}
	if(!tested)
		test.Printf(_L("Skipped T_FRAGMENTDP on drive %c\n"), path[0]);
	return tested;
	}


void DoTestF(TInt aDrvNum, TBool aNand)
	{
	TInt pos=0;
	TInt size, size1;
	TInt r;
	TFileName fileName;

	test.Next(_L("Testing Fragmentation of writes to writable drives in paging media"));
	if(aNand)
		test.Printf(_L("Testing on NAND\n"));

    fileName.Format(_L("Testing drive %c:\n"), 'A'+aDrvNum);
    test.Printf(fileName);

	_LIT(KTPagedCpp, "Z:\\test\\TEST_PAGED.cpp");
	_LIT(KTUnpagedCpp, "Z:\\test\\TEST_UNPAGED.CPP");

	if(TheFs.IsFileInRom(KTPagedCpp) != NULL && TheFs.IsFileInRom(KTUnpagedCpp) != NULL)	// .oby must include these files
		{
		RFile f;
		r=f.Open(TheFs,KTUnpagedCpp,EFileStream);
		test(r==KErrNone);
		r=f.Seek(ESeekAddress,pos);
		test(r==KErrNone);
		TText8* ptrPos=*(TText8**)&pos;			// start address of unpaged file in ROM
		test.Printf(_L("Start address of section to copy 0x%x\n"), ptrPos);

		r=f.Size(size);							// size of unpaged file
		test(r==KErrNone);
		size+=((~(size&0xf)&0xf)+1);			// adjust for ROM alignement (EABI, 8 bytes)
		f.Close();

		r=f.Open(TheFs,KTPagedCpp,EFileStream);
		test(r==KErrNone);
		r=f.Size(size1);							// size of paged file
		test(r==KErrNone);
		size1+=((~(size1&0xf)&0xf)+1);			// adjust for ROM alignement (EABI, 8 bytes)
		f.Close();

		size+=size1;
		test.Printf(_L("Set descriptor with size %d (paged+unpaged sections+ROMFS padding)\n"), size);
		TPtrC8 ptr(ptrPos,size);

		fileName.Format(_L("%c:\\TestFragFile.bin"), aDrvNum+'A');
		TheFs.Delete(fileName); //-- just in case

		test.Printf(_L("Create and open destination file\n"));
		r = CreateEmptyFile(TheFs, fileName, (size));		// create file to hold both sizes
		test(r == KErrNone);
		r=f.Open(TheFs,fileName,EFileRead|EFileWrite);
		test(r == KErrNone);

		test.Printf(_L("Attempt to flush paged section\n"));
		TInt r=UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
		if(r==KErrNotSupported)
			test.Printf(_L("Not Supported\n"));

		test.Printf(_L("Write paged and unpaged sections, synchronoulsy\n"));
		r=f.Write(ptr);
		test(r==KErrNone);

		test.Printf(_L("Read back and compare\n"));
		pos=0;
		r=f.Seek(ESeekStart,pos);
		test(r==KErrNone);
		TUint end=(TUint)ptrPos+(size);
		TBuf8<1024> readBuf;
		TPtrC8 memBuf(ptrPos,1024);

		while((TUint)ptrPos+1024<end)
			{
			r=f.Read(readBuf);
			test(r==KErrNone);
			test(readBuf.Length()==readBuf.MaxLength());
			if(memBuf!=readBuf)
				{
				test.Printf(_L("Failed on descriptor starting at address %x\n"), ptrPos);
				test(0);
				}
			ptrPos+=1024;
			memBuf.Set(ptrPos,1024);
			}
		r=f.Read(readBuf);
		test(r==KErrNone);
		test(readBuf.Length()==(TInt)(end-(TUint)ptrPos));
		memBuf.Set(ptrPos,(end-(TUint)ptrPos));
		if(memBuf!=readBuf)
			{
			test.Printf(_L("Failed on descriptor starting at address %x\n"), ptrPos);
			test(0);
			}
		f.Close();
		}
	else
		{
		test.Printf(_L("Required test files not present\n"));
		test(0);
		}
	}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
LOCAL_C TInt ConcurrThread(TAny* aArg);
/*
  This equally unimpressive looking test further exercises the fragmentation of write requests 
  carried out by the Local Media subsystem. This time write requests where the request source is
  in paged out ROM are issued concurrently.
  By having concurrent writes it indirectly tests both page in and fragment deferral mechaninsms.
*/

LOCAL_C void TestConcurrentFragmentation()
	{
	// concurrently write from paged out ROM addresses to either files in separate writebla partitions or different locations in the same partition
	TDriveList driveList;
	TDriveInfo driveInfo;
	TBool concurr=EFalse;

	TFileName path;
	TInt r=TheFs.SessionPath(path);
	test(r==KErrNone);
	TInt drvNum;
	r=RFs::CharToDrive(path[0],drvNum);
	test(r==KErrNone);

	r=TheFs.DriveList(driveList);
    test(r == KErrNone);

	test(TheFs.Drive(driveInfo, drvNum) == KErrNone);

	//-- select suitable drives for the testing. They shall be writable drives on a media that services paging
	if((driveInfo.iMediaAtt&KMediaAttPageable) && !(driveInfo.iMediaAtt&KMediaAttWriteProtected))
		{
		for (TInt drvNum1=drvNum+1; drvNum1<KMaxDrives; drvNum1++)	// if yes search for more drives suitable for concurrent fragmentation
			{
			if(!driveList[drvNum1])
				continue;   //-- skip unexisting drive

			TDriveInfo driveInfo2;	// for second drive
			test(TheFs.Drive(driveInfo2, drvNum1) == KErrNone);
			if ((driveInfo2.iMediaAtt&KMediaAttPageable) && 
				!(driveInfo2.iMediaAtt&KMediaAttWriteProtected) &&
				(driveInfo.iType == driveInfo2.iType))
				{
				DoTestC(drvNum, drvNum1, (driveInfo.iType==EMediaNANDFlash)?(TBool)ETrue:(TBool)EFalse);		// test concurrent
				concurr=ETrue;
				}
			}
		}
	if(!concurr)
		test.Printf(_L("Skipped concurrent test\n"));
	}


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


void DoTestC(TInt aDrvNum, TInt aNotherDrvNum, TBool aNand)
	{
	TInt pos=0;
	TInt size=0;
	TInt r;
	TRequestStatus logonStat;
	RThread concurrThread;
	TInt locDriveNumber;
	TBusLocalDrive drive;
	TLocalDriveCapsV4 driveCaps;
	SDeferStats stats;

	test.Next(_L("Testing concurrent Fragmentation of writes on the same media"));
	if(aNand)
		test.Printf(_L("Testing on NAND\n"));
	test.Printf(_L("Testing on writable drives %c and %c\n"), 'A'+aDrvNum,'A'+aNotherDrvNum);

	_LIT(KTPagedCpp, "Z:\\test\\TEST_PAGED.cpp");
	_LIT(KTPaged1Cpp, "Z:\\test\\TEST_PAGED1.cpp");

	if(TheFs.IsFileInRom(KTPagedCpp) != NULL && TheFs.IsFileInRom(KTPaged1Cpp) != NULL)	// .oby must include these files
		{
		RFile f;
		r=f.Open(TheFs,KTPagedCpp,EFileStream);		// source 1
		test(r==KErrNone);
		r=f.Seek(ESeekAddress,pos);
		test(r==KErrNone);
		TText8* ptrPos=*(TText8**)&pos;			// start address of paged file 1 in ROM
		test.Printf(_L("Main thread->Start address of paged out file 1 0x%x\n"), ptrPos);
		r=f.Size(size);							// size of paged file 1
		test(r==KErrNone);
		f.Close();

		TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
		TUint fsize=Min(KMuliplySize*size,romHeader->iPageableRomSize);

		test.Printf(_L("Main thread->Set descriptor with size %d to point to paged out file 1 +...\n"), fsize);
		TPtrC8 ptr(ptrPos,fsize);

		Buffer.SetLength(fsize);
		TPtr8 readBuf(&Buffer[0],fsize,fsize);

		test.Printf(_L("Create and resume concurrent thread\n"));
		const TInt KHeapSize=0x2000;

		locDriveNumber = GetLocDrvNumber(aNotherDrvNum);
		TInt r = concurrThread.Create(_L("ConcurrentWriteThread"),ConcurrThread,KDefaultStackSize,KHeapSize,KHeapSize,(TAny*)locDriveNumber);
		test(r==KErrNone);
		concurrThread.Logon(logonStat);

		locDriveNumber = GetLocDrvNumber(aDrvNum);
		test.Printf(_L("Connect to local drive %d\n"),locDriveNumber);
		TBool changeFlag = EFalse;
		r = drive.Connect(locDriveNumber,changeFlag);
		TPckg<TLocalDriveCapsV4>	capsPack(driveCaps);
		drive.Caps(capsPack);
		test(r == KErrNone);

		test(WriteSemaphore.CreateLocal(0)==KErrNone);

		// try to ensure there is no other thread activity as this may prevent the 
		// large write from being pre-empted by the ConcurrentWriteThread
		test.Printf(_L("Waiting 2 secs for file server threads to quieten down...."));
		User::After(2000000);

		concurrThread.Resume();

		WriteSemaphore.Wait();
		WriteSemaphore.Signal();

		// long write...
//		test.Printf(_L("Starting file 1 write\n"));	
		r = drive.Write(0,ptr);
		test(r==KErrNone);

		test.Printf(_L("Main thread->Write 1 completed\n"));

		if(aNand)
			{
			test.Printf(_L("Read stats\n"));
			TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
	 		test(drive.ControlIO(KNandGetDeferStats, statsBuf, 0)==KErrNone);
			test.Printf(_L("Fragmentation clashes %d Fragmentation deferrals %d Page In deferrals %d Other deferrals %d\n"),stats.iClashFragmenting,  stats.iNormalFragmenting, stats.iPageOther, stats.iNormalOther);
			}

		test.Printf(_L("Read back file 1 and compare\n"));
		r = drive.Read(0,fsize,readBuf);
		test(r==KErrNone);
		test(ptr==readBuf);
		test.Printf(_L("Verify file 1 OK\n"));
		drive.Disconnect();

		WriteSemaphore.Signal();
		User::WaitForRequest(logonStat);
		test(logonStat==KErrNone);
		concurrThread.Close();
		WriteSemaphore.Close();

		silentFormat(aDrvNum);
		silentFormat(aNotherDrvNum);
		}
	else
		{
		test.Printf(_L("Required test files not present\n"));
		test(0);
		}
	}

	
GLDEF_D RFs TheFsT;
GLDEF_D RTest testT(_L("T_CONCURRENT_WRITE_THREAD"));

LOCAL_C TInt ConcurrThread(TAny* aArg)
	{
	// the whole test is dodgy and hangs if this thread fails an assert,
	// so at least make sure thread panic takes out whole test process...
	User::SetCritical(User::EProcessCritical);

	RFile f;
	TInt pos=0;
	TInt size=0;
	TInt locDriveNumber;
	TBusLocalDrive drive;
	TLocalDriveCapsV4 driveCaps;
	SDeferStats stats;
	RThread thisThread;

	TInt r = TheFsT.Connect();
	_LIT(KTPaged1Cpp, "Z:\\test\\TEST_PAGED1.cpp");
	r=f.Open(TheFsT,KTPaged1Cpp,EFileStream);		// source 2
	testT(r==KErrNone);
	r=f.Seek(ESeekAddress,pos);
	testT(r==KErrNone);
	TText8* ptrPos=*(TText8**)&pos;			// start address of paged file 2 in ROM
	testT.Printf(_L("ConcurrThread->Start address of paged out file 2 0x%x\n"), ptrPos);
	r=f.Size(size);							// size of paged file 2
	testT(r==KErrNone);
	f.Close();

	testT.Printf(_L("ConcurrThread->Set descriptor with size %d to point to paged out file 2\n"), size);
	TPtrC8 ptr(ptrPos,size);

	TPtr8 readBuf(&Buffer[0],size,size);

	locDriveNumber = (TInt)aArg;
	testT.Printf(_L("Connect to local drive %d\n"),locDriveNumber);
	TBool changeFlag = EFalse;
	r = drive.Connect(locDriveNumber,changeFlag);
	TPckg<TLocalDriveCapsV4>	capsPack(driveCaps);
	drive.Caps(capsPack);
	testT(r == KErrNone);

	if (driveCaps.iType == EMediaNANDFlash)
		{
		testT.Printf(_L("Zero stats\n"));
		TPtr8 statsBuf((TUint8*) &stats, sizeof(stats));
 		testT(drive.ControlIO(KNandGetDeferStats, statsBuf, 0)==KErrNone);
		}

	r=UserSvr::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
	if(r==KErrNotSupported)
		testT.Printf(_L("ConcurrThread->Flushing of paging not Supported\n"));

	// pause one second to make sure main thread has executed WriteSemaphore.Wait();
	User::After(1000000);

	WriteSemaphore.Signal();
	WriteSemaphore.Wait();
	// up our priority
	thisThread.SetPriority(EPriorityMore);

	// wait a very short time to give the other thread a better chance to initiate the write
	User::After(1);	
//	testT.Printf(_L("Starting file 2 write\n"));

	// write
	r = drive.Write(0,ptr);
	testT(r==KErrNone);
	// read back
	r = drive.Read(0,size,readBuf);
	testT(r==KErrNone);
	// erase
	r=drive.Format(0,size);
	testT(r==KErrNone);

	testT.Printf(_L("ConcurrThread->Write of file 2 completed\n"));

	WriteSemaphore.Wait();
	testT.Printf(_L("Read back file 2 and compare\n"));
	testT(ptr==readBuf);
	testT.Printf(_L("Verify file 2 OK\n"));

	drive.Disconnect();
	TheFsT.Close();
	return KErrNone;
	}

#endif // #if defined(_DEBUG) || defined(_DEBUG_RELEASE)

//--------------------------------------------------------

/**
    Create an empty file of specified size.
    @param  aFs		    ref. to the FS
    @param  aFileName   name of the file
    @param  aFileSize   size of the file to be created
    @return    KErrNone on success, system-wide error code otherwise
*/
TInt CreateEmptyFile(RFs& aFs, const TDesC& aFileName, TUint aFileSize)
	{
    RFile   file;
	TInt    nRes;

	nRes = file.Create(aFs, aFileName, EFileWrite);
    if(nRes != KErrNone)
        return nRes;

	nRes = file.SetSize(aFileSize);
    if(nRes != KErrNone)
        return nRes;

    file.Close();

    return KErrNone;
	}

TInt GetLocDrvNumber(TInt aDrvNo)
	{
	test.Printf(_L("GetLocDrvNumber\r\n"));
	TInt locDriveNumber;
	RFile file;
	TBuf<256> fileName;	
	fileName.Append((TChar)('A'+aDrvNo));
	fileName+=_L(":\\f32-tst\\");
	TInt r=TheFs.MkDirAll(fileName);
	test(r==KErrNone || r== KErrAlreadyExists);
	fileName += _L("maggots.txt");
	r=file.Replace(TheFs,fileName,EFileWrite|EFileWriteDirectIO);
	if (r!=KErrNone)
		test.Printf(_L("Error %d: file '%S' could not be created\n"),r,&fileName);
	test(r==KErrNone);
	r=file.Write(_L8("Writhing bundles of maggots, this was truly their finest hour"));
	if (r!=KErrNone)
		test.Printf(_L("Error %d: could not write to file\n"),r);
	test(r==KErrNone);

	SBlockMapInfo info;
	TInt64 start=0;
	r=file.BlockMap(info,start, -1,ETestDebug);
	if (r!=KErrNone && r!=KErrCompletion)
		test.Printf(_L("Error %d: could not obtain block map\n"),r);
	test(r==KErrNone || r==KErrCompletion);
	locDriveNumber=info.iLocalDriveNumber;
	test.Printf(_L("From drive: %c to Local drive %d\r\n"), aDrvNo+'A',locDriveNumber);
	file.Close();
	return locDriveNumber;
	}

GLDEF_C void CallTestsL()
	{
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	if(!romHeader->iPageableRomStart)
		{
		test.Printf(_L("Test not supported (not a paged ROM)\n"));
		return; // Not a paged ROM, skip test
		}
	test.Title();

	TBool r=TestSimpleFragmentation();
	if(!r)
		return;
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TestConcurrentFragmentation();
#endif
	}
