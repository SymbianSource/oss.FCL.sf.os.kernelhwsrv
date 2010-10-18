// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include <e32math.h>

#include "fat_utils.h"
#include "t_server.h"
#include "t_chlffs.h"

using namespace Fat_Test_Utils;

RTest test(_L("t_falsespace"));

const TInt KNumberThreads=2;
const TInt KHeapSize=0x2000;

//-- this is taken from the file server. Hardcoded constant that specifies session limits on reserving the drive space
const TInt KMaxSessionDriveReserved	=0x10000; 

static TInt RsrvSpaceThread(TAny* aArg);
static TInt SessCloseThread(TAny* aArg);
static void GetFreeDiskSpace(TInt64 &aFree);


TInt gCount;		//count of files used to fill up the disk
TInt gTestDrive;	//drive number of the drive currently being tested

TChar gCh;

_LIT(KBasePath,"\\F32-TST\\FILLDIR\\");
_LIT(KBaseName,"\\F32-TST\\FILLDIR\\FILE");

_LIT(KTestFile,"?:\\test.txt");
_LIT8(KTestData, "12345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678123456781234567812345678");
_LIT(KDir, "?:\\adodgydir\\");
_LIT(KDrv,"?:\\");	
_LIT(KNewName,"?:\\newname.txt");


void FormatDrive()
{
    TInt nRes;

    
    #if 0
    //-- FAT32 SPC:1; for the FAT32 testing on the emulator 
    TFatFormatParam fp;
    fp.iFatType = EFat32;
    fp.iSecPerCluster = 1;

	nRes = FormatFatDrive(TheFs, gTestDrive, ETrue, &fp);	
    #else

    nRes = FormatFatDrive(TheFs, gTestDrive, ETrue);	

    #endif

    test_KErrNone(nRes);
}

void SynchronousClose(RFs &aSession)
	{
	TRequestStatus s;
	aSession.NotifyDestruction(s);
	test(s.Int()==KRequestPending);
	aSession.Close();
	User::WaitForRequest(s);
	}

//-----------------------------------------------------------------------------
/**
    Get a cluster size for the currently mounted FS (if it supports clusters). 
    @return 0 if there was an error (e.g. cluster size query is not supported), otherwise a cluster size
*/
static TUint32 FsClusterSize()
{
    TVolumeIOParamInfo volIop;
    TInt nRes = TheFs.VolumeIOParam(gTestDrive, volIop);

    if(nRes != KErrNone || volIop.iClusterSize < 512 || !IsPowerOf2(volIop.iClusterSize))
        {
        test.Printf(_L("FsClusterSize() The FS hasn't reported a cluster size\n"));
        return 0;
        }
        
    return volIop.iClusterSize;
}

static TInt CreateFileX(const TDesC& aBaseName,TInt aX, TInt aFileSize)
//
// Create a large file. Return KErrEof or KErrNone
//
	{

	TBuf<128> fileName=aBaseName;
	fileName.AppendNum(aX);
	RFile file;

	TInt r=file.Replace(TheFs,fileName,EFileWrite);
	if (r==KErrDiskFull)
		return(r);
	if (r!=KErrNone)
		{
		test.Printf(_L("ERROR:: Replace returned %d\n"),r);
		test(0);
		return(KErrDiskFull);
		}

	if (!IsTestingLFFS())
		r=file.SetSize(aFileSize);
	else
		{
    	TBuf8<1024> testdata(1024);
    	TInt count=(aFileSize/testdata.Length());
    	r=KErrNone;
    	while (count-- && r==KErrNone) 
        	r=file.Write(testdata);
		}
	if (r==KErrDiskFull)
		{
		file.Close();
		return(r);
		}
	if (r!=KErrNone)
		{
		test.Printf(_L("ERROR:: SetSize/Write returned %d\n"),r);
		test(0);
		//test.Getch();
		file.Close();
		return(KErrDiskFull);
		}

	file.Close();

	test.Printf(_L("Created file %d size %d\n"),aX,aFileSize);
	return(KErrNone);
	}

LOCAL_C TInt DeleteFileX(const TDesC& aBaseName,TInt aX)
//
// Delete a large file
//
	{
	TBuf<128> fileName=aBaseName;
	fileName.AppendNum(aX);
	return TheFs.Delete(fileName);
	}


static void FillUpDisk()
//
// Test that a full disk is ok
//
	{
	test.Next(_L("Fill disk to capacity\n"));
	TInt r=TheFs.MkDirAll(KBasePath);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	gCount=0;
	TFileName sessionPath;
	r=TheFs.SessionPath(sessionPath);
	test_KErrNone(r);
	TBuf<128> fileName=KBaseName();
	
	TInt64 freespace=0;
	TInt64 freespaceBeforeScanDrive = 0;
	TInt64 freespaceAfterScanDrive = 0;
	
	do
		{
		GetFreeDiskSpace(freespace);
		TInt fillfilesize=0;
		if (I64HIGH(freespace))
			fillfilesize=KMaxTInt;
		else
			fillfilesize=I64LOW(freespace)* 7/8;

		FOREVER
			{
			TInt r=CreateFileX(fileName,gCount,fillfilesize);
			if (r==KErrDiskFull)
				{
				if(fillfilesize <= 2)
					break;
				else
					fillfilesize=fillfilesize/2;
				}
			test_Value(r, r == KErrNone || r==KErrDiskFull);
			if(r==KErrNone)
				gCount++;
			}

		r=TheFs.CheckDisk(fileName);
		if (r!=KErrNone && r!=KErrNotSupported)
			{
			test.Printf(_L("ERROR:: CheckDisk returned %d\n"),r);
			test(0);

			}

		// Test that scan drive passes on a full disk
		// DEF071696 - KErrCorrupt on Scan Drive 
		GetFreeDiskSpace(freespaceBeforeScanDrive);
		test.Printf(_L("Before ScanDrive freeSpace = %08X:%08X\n"),
			I64HIGH(freespaceBeforeScanDrive), I64LOW(freespaceBeforeScanDrive));
		r = TheFs.ScanDrive(fileName);
		if (r!=KErrNone && r!=KErrNotSupported)
			{
			test.Printf(_L("ScanDrive returned %d\n"), r);
			test(0);
			}
		GetFreeDiskSpace(freespaceAfterScanDrive);
		test.Printf(_L("After ScanDrive freeSpace = %08X:%08X\n"),
			I64HIGH(freespaceAfterScanDrive), I64LOW(freespaceAfterScanDrive));
		}
		while (freespaceBeforeScanDrive != freespaceAfterScanDrive );

	gCount--;
	}

static void GetFreeDiskSpace(TInt64 &aFree)
//
//	Get free disk space
//
	{
	TVolumeInfo v;

	TInt r=TheFs.Volume(v,gTestDrive);
	test_KErrNone(r);
	aFree=v.iFree;
	}


static void Test1()
//
//	Test the API fundamentaly works for one session
//
	{
	test.Next(_L("Test Disk Space reserve APIs\n"));
	TInt r=0;
	
    FormatDrive();
	
	TInt64 free2;
	TInt64 free1; 
	TInt64 diff;

	r=TheFs.GetReserveAccess(gTestDrive);
	test_Value(r, r == KErrPermissionDenied);
	
	//make sure nothing odd happens if we didnt already have access
	r=TheFs.ReleaseReserveAccess(gTestDrive);
	test_KErrNone(r);

	
	GetFreeDiskSpace(free2);

	r=TheFs.ReserveDriveSpace(gTestDrive,0x1000);
	test_KErrNone(r);

	GetFreeDiskSpace(free1);
	diff = free2 - free1;
	test(I64INT(diff) > 0xfe0 && I64INT(diff) < 0x1100); 
	
	r=TheFs.GetReserveAccess(gTestDrive);
	test_KErrNone(r);

	GetFreeDiskSpace(free1);
	TInt64 temp = free2-free1;
	test(I64INT(temp)>(-0x90) && I64INT(temp)<0x90);
	
	r=TheFs.ReleaseReserveAccess(gTestDrive);
	test_KErrNone(r);
	GetFreeDiskSpace(free1);

	diff = free2 - free1;
	test(I64INT(diff) > 0xfe0 && I64INT(diff) < 0x1100);
	
	
	//test reallocation of reserved space is possible
	r=TheFs.ReserveDriveSpace(gTestDrive,0x2000);
	test_KErrNone(r);

	//test upper limit of reserved space 
	r=TheFs.ReserveDriveSpace(gTestDrive,0x2000000);
	test_Value(r, r == KErrArgument);

	r=TheFs.ReserveDriveSpace(gTestDrive,0);
	test_KErrNone(r);
	
	r=TheFs.GetReserveAccess(gTestDrive);
	test_Value(r, r == KErrPermissionDenied);

	//make sure nothing odd happens if we didnt already have access
	r=TheFs.ReleaseReserveAccess(gTestDrive);
	test_KErrNone(r);
	
	r=TheFs.ReserveDriveSpace(gTestDrive,-45);
	test_Value(r, r == KErrArgument);
	}


static void Test2()
//
//	Test multiple sessions, ensure the drive limit is not exceeded
//	
	{

	test.Next(_L("Test Session and total reserve limits\n"));
	
    FormatDrive();
	
	TInt i=0;
	TInt r=0;
	RFs sessions[17];
	TVolumeInfo v;

	//Connect Sessions
	for(i=0; i<17; i++)
		{
		r = sessions[i].Connect();
		test_KErrNone(r);
		}

	test.Next(_L("Test breaching sesson reserve limit\n"));
	r=sessions[0].ReserveDriveSpace(gTestDrive,0x10001);
	test_Value(r, r == KErrArgument);

	//Get Volume Free Space
	r = sessions[0].Volume(v, gTestDrive);

	if(v.iFree > 0x100000)
		{
		test.Next(_L("Test breaching drive reserve limit\n"));

		for (i=0; i<16; i++)
			{
			r=sessions[i].ReserveDriveSpace(gTestDrive,0x10000);
			test_KErrNone(r);
			}

		//The straw
		r=sessions[16].ReserveDriveSpace(gTestDrive,0x10);
		test_Value(r, r == KErrTooBig);
		}
	else
		{
		test.Printf(_L("Drive too small: breaching drive reserve limit test skipped\n"));
		test.Next(_L("Testing exhausting available drive free space instead\n"));

		for(i=0; (v.iFree -= 0x10000) >= 0; i++)
			{
			r=sessions[i].ReserveDriveSpace(gTestDrive,0x10000);
			test_KErrNone(r);
			}

		//The straw
		r=sessions[i].ReserveDriveSpace(gTestDrive,0x10000);
		test_Value(r, r == KErrDiskFull);
		}

	//Close Sessions
	for(i=0; i<17; i++)
		{
		SynchronousClose(sessions[i]);
		}
	}

static void Test3()
//
//	Test session cleanup
//		
	{
	test.Next(_L("Test session close and clean up of resrved space\n"));

	FormatDrive();
	
	RFs fs1;
	RFs fs2;
	TInt64 free2(0);
	TInt64 free1(0); 
	TInt64 diff(0);
	
	TInt r=0;
	r = fs1.Connect();
	test_KErrNone(r);
	r = fs2.Connect();
	test_KErrNone(r);

	GetFreeDiskSpace(free1);

	r=fs1.ReserveDriveSpace(gTestDrive,0x10000);
	test_KErrNone(r);
	r=fs2.ReserveDriveSpace(gTestDrive,0x10000);
	test_KErrNone(r);

	GetFreeDiskSpace(free2);
	diff = free1 - free2;
	test(I64INT(diff)>0x1FBD0 && I64INT(diff)<0x21000); 

	SynchronousClose(fs1);

	GetFreeDiskSpace(free2);
	diff = free1-free2;
	test(I64INT(diff)>0xFA00 && I64INT(diff)<0x103C4); 

	r = fs1.Connect();
	test_KErrNone(r);

	GetFreeDiskSpace(free1);
	diff= free1-free2;
	test(I64INT(diff)== 0 || I64INT(diff)<0xFA0 ); 

	r=fs1.ReserveDriveSpace(gTestDrive,0x10000);
	test_KErrNone(r);

	GetFreeDiskSpace(free2);
	diff = free1 - free2;
	test(I64INT(diff)>0xFA00 && I64INT(diff)<0x103C4); 

	// Make sure no reserve space is allocated
	r=fs1.ReserveDriveSpace(gTestDrive,0);
	test_KErrNone(r);
	r=fs2.ReserveDriveSpace(gTestDrive,0);
	test_KErrNone(r);

	// Now fill up the disk
	FillUpDisk();
	
	// Should fail as there is no space
	r=fs1.ReserveDriveSpace(gTestDrive,0x10000);
	test_Value(r, r == KErrDiskFull);

	SynchronousClose(fs1);
	SynchronousClose(fs2);
	}


static void Test4()
//
//	Test real out of disk space conditions i.e. properly run out of disk space and try to 
//	reserve an area etc
//	
	{
	test.Next(_L("Test Filling disk and using APIs\n"));

	if(IsTestingLFFS())
		{
		//-- This test is not valid for LFFS, because free space on this FS can change itself because of some internal FS activities
		test.Printf(_L("This test is inconsistent on LFFS\n"));
		return;
		}
	

    const TInt KThreshold = 0x10000;
	
    //-- check that the cluster size doesn't exceed the max. limit
    const TInt KClusterSz = FsClusterSize();
    if(KClusterSz > KThreshold)
        {
        test.Printf(_L("The cluster size(%d) is bigger than threshold to test (%d)! Skipping the test!\n"), KClusterSz, KThreshold);
		return;
		}
	
    FormatDrive();

	RFs fs;
	TInt r=fs.Connect();
	test_KErrNone(r);
	TInt64 freeA(0);
	TInt64 freeB(0);
	RFile file;

									//start with known amount of space

	//create a single file to use for futher tests
	TBuf<20> buf;
	buf=KTestFile;
	buf[0]=(TUint16)gCh;

	r=file.Replace(fs, buf, EFileWrite);
	test_KErrNone(r);

	r=file.Write(KTestData());
	test_KErrNone(r);

	file.Close();

	r=fs.ReserveDriveSpace(gTestDrive,KThreshold);		//reserve some disk space
	test_KErrNone(r);
		
	FillUpDisk();									//fill up the disk

	TVolumeInfo v;									//get disk space
	r=fs.Volume(v,gTestDrive);
	test_KErrNone(r);
	freeA=v.iFree;

	r=fs.GetReserveAccess(gTestDrive);				//get access to reserve space
	test_KErrNone(r);

	r=fs.Volume(v,gTestDrive);						//get disk space
	test_KErrNone(r);
	freeB=v.iFree;
	
	r=fs.ReleaseReserveAccess(gTestDrive);			//release reserve space
	test_KErrNone(r);
	
	test(freeA == (freeB - KThreshold));				//test difference in space is equal to the amount reserved

	r=fs.Volume(v,gTestDrive);						//get disk space
	test_KErrNone(r);
	freeB=v.iFree;
	test(freeA == freeB);							//check reading is still correct
	
	TBuf <20> dir = KDir();
	dir[0]=(TUint16)gCh;
	r=fs.MkDir(dir);
	test_Value(r, r == KErrDiskFull);

	r=fs.MkDirAll(dir);
	test_Value(r, r == KErrDiskFull);

	TFileName temp;
	TBuf<5> drv = KDrv();
	drv[0]=(TUint16)gCh;
	r=file.Temp(fs, drv, temp, EFileWrite);
	test_Value(r, r == KErrDiskFull);

	r=file.Replace(fs, buf, EFileWrite);
	test_Value(r, r == KErrDiskFull);

	r=file.Create(fs, buf, EFileWrite);
	test_Value(r, r == KErrDiskFull);

	r=file.Open(fs, buf, EFileWrite);
	test_KErrNone(r);

	r=file.Write(128, KTestData());

	if ((gDriveCacheFlags & EFileCacheWriteOn) && (r == KErrNone))
		r = file.Flush();
	
	test_Value(r, r == KErrDiskFull);

	r=file.SetSize(0x1000);
	test_Value(r, r == KErrDiskFull);

	r=file.SetAtt(KEntryAttHidden,0); 
	test_Value(r, r == KErrDiskFull);

	TTime dtime;
	r=file.SetModified(dtime); 
	test_Value(r, r == KErrDiskFull);

	r=file.Set(dtime,KEntryAttHidden,0);
	test_Value(r, r == KErrDiskFull);

	r=file.Rename(buf);
	test_Value(r, r == KErrDiskFull);

	file.Close();


	// Test that we can create a temporary file & write to it after acquiring reserved access, 
	r=fs.GetReserveAccess(gTestDrive);				//get access to reserve space
	test_KErrNone(r);

	r=fs.Volume(v,gTestDrive);						//get disk space
	test_KErrNone(r);
	freeA = v.iFree;

	r=file.Temp(fs, drv, temp, EFileWrite);
	test_KErrNone(r);

	r = file.Write(KTestData());
	test_KErrNone(r);

	// If write caching is enabled, call RFs::Entry() to flush the file "anonymously"
	if ((gDriveCacheFlags & EFileCacheWriteOn) && (r == KErrNone))
		{
		r = file.Flush();
		test_KErrNone(r);
		}

	r=fs.Volume(v,gTestDrive);						//get disk space
	test_KErrNone(r);
	freeB = v.iFree;
	test (freeB < freeA);

	file.Close();

	r=fs.ReleaseReserveAccess(gTestDrive);			//release reserve space
	test_KErrNone(r);


	TBuf<20> newname =KNewName();
	newname[0]=(TUint16)gCh;
	r=fs.Rename(buf, newname);
	test_Value(r, r == KErrDiskFull);

	r=fs.Replace(buf, newname);
	test_Value(r, r == KErrDiskFull);

	r=fs.SetEntry(buf, dtime, KEntryAttHidden, 0);
	test_Value(r, r == KErrDiskFull);

	r=fs.CreatePrivatePath(gTestDrive);
	test_Value(r, r == KErrDiskFull);

	r=fs.SetVolumeLabel(_L("Moooo"), gTestDrive);
	test_Value(r, r == KErrDiskFull);	

	r=fs.SetModified(buf, dtime);
	test_Value(r, r == KErrDiskFull);	

	SynchronousClose(fs);
	}

	

static void Test5()
//
//
//
	{
	test.Next(_L("Test Session limits\n"));

	if(IsTestingLFFS())
		{
		//-- This test is not valid for LFFS, because free space on this FS can change itself because of some 
        //-- internal FS activities
		test.Printf(_L("This test is inconsistent on LFFS\n"));
		return;
		}


	RFs fs1;
	RFs fs2;
	TInt r=KErrNone;

	r=fs1.Connect();
	test_KErrNone(r);
	r=fs2.Connect();
	test_KErrNone(r);

	FormatDrive();

	r=fs1.ReserveDriveSpace(gTestDrive,0x10000);		
	test_KErrNone(r);

	r=fs2.ReserveDriveSpace(gTestDrive,0x10000);		
	test_KErrNone(r);

	FillUpDisk();									

	r=fs1.GetReserveAccess(gTestDrive);				
	test_KErrNone(r);

	TBuf<20> dir = KDir();
	dir[0]=(TUint16)gCh;


	r=fs2.MkDir(dir);
	test_Value(r, r == KErrDiskFull);

	r=fs1.ReserveDriveSpace(gTestDrive,0); //can not release reserve space while you have reserve access
	test_Value(r, r == KErrInUse);

	r=fs1.ReleaseReserveAccess(gTestDrive);				
	test_KErrNone(r);

	r=fs1.ReserveDriveSpace(gTestDrive,0); 
	test_KErrNone(r);

	r=fs2.MkDir(dir);
	test_KErrNone(r);

	SynchronousClose(fs1);
	SynchronousClose(fs2);
	}

static TInt RsrvSpaceThread(TAny* aArg)
	{
	TInt r=KErrNone;
	TInt64 fr1;
	TInt64 fr2;
	TInt64 diff;

	TVolumeInfo v;
	r=((RFs*)aArg)->Volume(v,gTestDrive);
	if(r!=KErrNone)
		return(r);

	fr1=v.iFree;

	r=((RFs*)aArg)->ReserveDriveSpace(gTestDrive,0x10000); 
	if(r!=KErrNone)
		return(r);

	r=((RFs*)aArg)->Volume(v,gTestDrive);
	if(r!=KErrNone)
		return(r);
	fr2=v.iFree;
	
	diff=fr1-fr2;
	if(!(I64INT(diff)> 0xef38 && I64INT(diff)<0xf100))
		return(KErrGeneral);
	return r;
	}

static TInt SessCloseThread(TAny* aArg)
	{
	TInt r=KErrNone;
	TInt64 fr1;
	TInt64 fr2;
	TInt64 diff;

	TVolumeInfo v;
	r=((RFs*)aArg)->Volume(v,gTestDrive);
	if(r!=KErrNone)
		return(r);
	fr1=v.iFree;

	((RFs*)aArg)->ReserveDriveSpace(gTestDrive,0x1000);
	
	r=((RFs*)aArg)->Volume(v,gTestDrive);
	if(r!=KErrNone)
		return(r);
	fr2=v.iFree;

	diff=fr2-fr1;
	if(!(I64INT(diff)> 0xef38 && I64INT(diff)<0xf100))
		return(KErrGeneral);

	SynchronousClose(*((RFs*)aArg));

	return r;
	}

static void Test6()
//
//	Test sharabale session
//
	{
	
	test.Next(_L("Test sharable session\n"));

	RFs fsess;
	TInt r=KErrNone;
	TInt64 free1(0);
	TInt64 free2(0);
	TInt64 diff(0);
	RThread t[KNumberThreads];
	TRequestStatus tStat[KNumberThreads];

	r=fsess.Connect();
	test_KErrNone(r);

	FormatDrive();

	r= fsess.ShareAuto();
	test_KErrNone(r);

	GetFreeDiskSpace(free1);

	fsess.ReserveDriveSpace(gTestDrive,0x1000);
		
	r = t[0].Create(_L("Sub_Thread1"),RsrvSpaceThread,KDefaultStackSize,KHeapSize,KHeapSize,&fsess); 
	test_KErrNone(r);

	t[0].Rendezvous(tStat[0]);
	t[0].Resume();

	User::WaitForRequest(tStat[0]);

	t[0].Close();
	test(tStat[0]==KErrNone);

	r = t[1].Create(_L("Sub_Thread2"),SessCloseThread,KDefaultStackSize,KHeapSize,KHeapSize,&fsess); 
	test_KErrNone(r);

	t[1].Rendezvous(tStat[1]);
	t[1].Resume();

	User::WaitForRequest(tStat[1]);

	t[1].Close();
	test(tStat[1]==KErrNone);

	GetFreeDiskSpace(free2);

	diff = free1-free2;
	test(I64INT(diff)== 0 || I64INT(diff)<0xFA0 );
	}


static void Test7()
//
// Tests notifier events for sessions with and without reserved access
//
	{
	test.Next(_L("Test reserved access notification\n"));

	if(IsTestingLFFS())
		{
		// This test is not valid for LFFS...
		test.Printf(_L("Test reserved access notification not run for LFFS\n"));
		return;
		}

	FormatDrive();

    //-- find out the cluster size
    const TInt KClusterSz = FsClusterSize();
    if(!IsPowerOf2(KClusterSz))
        {
        test.Printf(_L("The FS hasn't reported a cluster size. The test is inconsistent, skipping\n"));
        return;
        }

    //-- check that the cluster size doesn't exceed the max. limit
    if(KClusterSz > KMaxSessionDriveReserved)
        {
        test.Printf(_L("The cluster size(%d) is bigger than reserve limit (%d)! Skipping the test!\n"), KClusterSz, KMaxSessionDriveReserved);
        return;
        }

    
    const TInt resSpace = Max(0x1000, KClusterSz);

	RFs theNrm;
	RFs theRes;

	TInt err = theNrm.Connect();
	test(KErrNone == err);

	err = theRes.Connect();
	test(KErrNone == err);


	TInt64 freeSpace(0);
	GetFreeDiskSpace(freeSpace);

	RFs theTestSession;
	theTestSession.Connect();

	_LIT(KFileFiller, "?:\\t_falseSpaceFiller");
	TBuf<25> fileName;
	fileName = KFileFiller;
	fileName[0] = (TUint16)gCh;

	err = theTestSession.Connect();
	test_KErrNone(err);

	TVolumeInfo volInfo;
	theNrm.Volume(volInfo, gTestDrive);
	test(volInfo.iFree == freeSpace);

	err = theRes.ReserveDriveSpace(gTestDrive, resSpace);
	test(KErrNone == err);
	err = theRes.GetReserveAccess(gTestDrive);
	test(KErrNone == err);

	theRes.Volume(volInfo, gTestDrive);
	test(volInfo.iFree == freeSpace);

	theNrm.Volume(volInfo, gTestDrive);
	test(volInfo.iFree == freeSpace - resSpace);


    RFile theFile;

	//
	// Register the notifiers and verify that the only the "Normal"
	// and not the "Reserved" session is triggered.
	//
	TRequestStatus statNrm;
	TRequestStatus statRes;

	TInt64 threshold(freeSpace - resSpace*2);
	theNrm.NotifyDiskSpace(threshold, gTestDrive, statNrm);
	theRes.NotifyDiskSpace(threshold, gTestDrive, statRes);
	test((statNrm == KRequestPending) && (statRes == KRequestPending));


	//
	// Main part of the test starts here.
	// First we create a new file, then we increase its size to cause the
	// "Normal" notifier to trigger but not the "Reserved" notifier
	//
	err=theFile.Replace(theTestSession, fileName, EFileShareAny | EFileWrite);
	test_KErrNone(err);
	test((statNrm == KRequestPending) && (statRes == KRequestPending));

	// Neither notifier should be triggered here
	err = theFile.SetSize(resSpace);
	test(KErrNone == err);
	test((statNrm == KRequestPending) && (statRes == KRequestPending));

	// This should trigger the "Normal" notifier, but not the "Reserved" one
	err = theFile.SetSize(2*resSpace);
	test(KErrNone == err);
	test((statNrm == KErrNone) && (statRes == KRequestPending));


	//
	// Reset the "Normal" notifier then increase the amount of reserved space
	// on the drive. This should re-trigger the "Normal" notifier but leave
	// the "Reserved" notifier untouched.
	//
	theNrm.NotifyDiskSpace(threshold - resSpace, gTestDrive, statNrm);
	test((statNrm == KRequestPending) && (statRes == KRequestPending));

	err = theTestSession.ReserveDriveSpace(gTestDrive, resSpace * 3);
	if (err != KErrArgument)	// will have exceeded limit if resSpace = 32K
		{
		test_KErrNone(err);
		test((statNrm == KErrNone) && (statRes == KRequestPending));
		}

	//
	// All done - tidy up.
	//
	theFile.Close();
	theTestSession.Delete(fileName);
	theTestSession.Close();
	theNrm.Close();
	theRes.Close();
	}

LOCAL_C void TestForDEF142554()
    {
    test.Next(_L("Test for DEF142554: test RFile::Modified and RFile::Att when disk full\n"));
    
    Format(gTestDrive);
    
    TUint att;
    TTime time;
    
    RFs fs;
    TInt err = fs.Connect();
    test_KErrNone(err);

    RFile file;
    TBuf<20> fileName;
    fileName = KTestFile;
    fileName[0] = (TUint16)gCh;
    
    err = fs.ReserveDriveSpace(gTestDrive,0x10000); 
    test_KErrNone(err);

    err = file.Replace(fs, fileName, EFileWrite);
    test_KErrNone(err);

    err = file.Write(KTestData);
    test_KErrNone(err);
    
    err = file.Flush();
    test_KErrNone(err);
    
    file.Close();
    
    err = file.Open(fs, fileName, EFileRead);
    test_KErrNone(err);
    
    err = file.Att(att);
    test_KErrNone(err);
    
    err = file.Modified(time);
    test_KErrNone(err);
    
    file.Close();
    
    FillUpDisk();
    
    err = file.Open(fs, fileName, EFileRead);
    test_KErrNone(err);
    
    TUint att1;
    err = file.Att(att1);
    test_KErrNone(err);
    test(att1 == att);
    
    TTime time1;
    err = file.Modified(time1);
    test_KErrNone(err);
    test(time1 == time);
    
    file.Close();
    fs.Close();
    
    }


//-----------------------------------------------------------------------------

/**
    test creation of the the file that crosses 4G boundary on the FAT media

*/
static void TestFAT4G_Boundary()
	{
    const TInt64 K4Gig = 4*(TInt64)K1GigaByte;

	test.Next(_L("Test files crossing 4G boundary on FAT\n"));

    if(!Is_Fat32(TheFs, gTestDrive))
		{
		test.Printf(_L("This test requires FAT32. Skipping.\n"));
		return;
		}

    TVolumeInfo volInfo;
	
	TInt nRes = TheFs.Volume(volInfo,gTestDrive);
	test_KErrNone(nRes);
	
    if(volInfo.iSize < K4Gig+K1MegaByte)
		{
		test.Printf(_L("This test requires volume > 4G. Skipping.\n"));
		return;
		}
	
    //-- 1. format the volume
    FormatDrive();

    //-- find out media position of the data region start
    TFatBootSector bootSector;
    nRes = ReadBootSector(TheFs, gTestDrive, 0, bootSector);
    test_KErrNone(nRes);
    test(bootSector.IsValid());

    const TInt64 dataStartPos = bootSector.FirstDataSector() << KDefaultSectorLog2;
    const TInt64 lowRegion = K4Gig - dataStartPos - K1MegaByte; 


    //-- 2. create several empty files that take a bit less that 4gig
    //-- the drive is freshly formatted and the files will expand linearry
    _LIT(KBaseFN, "\\LargeFile");
    
    const TInt MaxDummyFiles = 5;
    const TUint32 DummyFileLen = (TUint32)(lowRegion / MaxDummyFiles);
	TInt i;
    for(i=0; i<MaxDummyFiles; ++i)
		{
        nRes = CreateFileX(KBaseFN, i, DummyFileLen); 
        test_KErrNone(nRes);
		}

    //-- 3. create a real file that crosses 4G boundary
    nRes = CreateCheckableStuffedFile(TheFs, KBaseFN, 5*K1MegaByte);
    test_KErrNone(nRes);
    
    test.Printf(_L("Verifying the file that crosses 4G boundary.\n"));

    nRes = VerifyCheckableFile(TheFs, KBaseFN);
    test_KErrNone(nRes);

	
	nRes = TheFs.Delete(KBaseFN);
	test_KErrNone(nRes);
    for(i=0; i<MaxDummyFiles; ++i)
	    {
        nRes = DeleteFileX(KBaseFN, i);
        test_KErrNone(nRes);
		}
	}

void TestRAMDriveNotification()
	{
	test.Next(_L("Verifying RFs::ReserveDriveSpace() triggers RFs::NotifyDiskSpace() events\n"));

	TInt64 freeSpace;
	GetFreeDiskSpace(freeSpace);
	test.Printf(_L("free space: 0x%Lx bytes\n"), freeSpace);

	// set a notification on half the amount we plan to reserve
	TInt reserve = 4096;
	TInt64 trigger = freeSpace - 2048;
	test.Printf(_L("setting notification for space to fall below: 0x%Lx bytes ... \n"), trigger);
	TRequestStatus stat;
	TheFs.NotifyDiskSpace(trigger, gTestDrive, stat);
	test_Value(stat.Int(), stat == KRequestPending);
	test.Printf(_L("ok\n"));

	// reserve the space and validate that this triggers the notification
	test.Printf(_L("reserving 0x%x bytes ...\n"), reserve);
	TInt r = TheFs.ReserveDriveSpace(gTestDrive, reserve);
	test_KErrNone(r);
	test.Printf(_L("ok\n"));

	test.Printf(_L("validating that the disk space notification triggered ...\n"));
	User::After(2000000);	// 2 seconds should be enough to cause the trigger
	test_Value(stat.Int(), stat == KErrNone);
	test.Printf(_L("ok\n"));
	}


//-----------------------------------------------------------------------------
/**
    Test that the reserving some drive space does not takes more space than required.
*/
void Test0()
{
    test.Next(_L("test ReserveDriveSpace threshold\n"));

    TInt nRes;
    TInt64 freespace=0;

    //-- 1. format the volume
    FormatDrive();

    GetFreeDiskSpace(freespace);
    const TInt64 freeSpace1 = freespace; //-- initial amount of free space on the volume

    const TInt KClusterSz = FsClusterSize();
    if(!IsPowerOf2(KClusterSz))
        {
        test.Printf(_L("The FS hasn't reported a cluster size. The test is inconsistent, skipping\n"));
        return;
        }

    //-- check that the cluster size doesn't exceed the max. limit
    if(KClusterSz > KMaxSessionDriveReserved)
        {
        test.Printf(_L("The cluster size(%d) is bigger than reserve limit (%d)! Skipping the test!\n"), KClusterSz, KMaxSessionDriveReserved);
        return;
        }


    //-- reserve exactly 1 cluster worth drive space.
    nRes = TheFs.ReserveDriveSpace(gTestDrive, KClusterSz);
    test_KErrNone(nRes);

    GetFreeDiskSpace(freespace);
    const TInt64 freeSpace2 = freespace;
    test((freeSpace1 - freeSpace2) == KClusterSz);

    //-- fill up a drive (it has a reserved space)
    FillUpDisk();

    //-- delete 1 file; 
    nRes = DeleteFileX(KBaseName, 0);
    test_KErrNone(nRes);

    //-- try to create a file with the size that is exacly the same as free space; it should succeed
    GetFreeDiskSpace(freespace);
    
    nRes = CreateEmptyFile(TheFs, _L("\\aaa1"), freespace);
    test_KErrNone(nRes);

    GetFreeDiskSpace(freespace);
    test(freespace == 0);

    //-- return the drive space to the system
	nRes = TheFs.ReserveDriveSpace(gTestDrive,0);
	test_KErrNone(nRes); 

    //-- release drive space
    nRes = TheFs.ReleaseReserveAccess(gTestDrive);
    test_KErrNone(nRes);

    GetFreeDiskSpace(freespace);
    test(freespace == KClusterSz);

    FormatDrive();
}

//-----------------------------------------------------------------------------

GLDEF_C void CallTestsL()
//
// Do tests relative to session path
//
	{
	//-- set up console output
	Fat_Test_Utils::SetConsole(test.Console());

	// If TESTFAST mode (for automated test builds) is set, don't run LFFS tests.
	if ((UserSvr::DebugMask(2) & 0x00000002) && IsTestingLFFS())
		{
		test.Printf(_L("TEST NOT RUN FOR LFFS DRIVE\n"));
		return;
		}

	//get the number of the drive we are currently testing
	TInt r=0;
	r=RFs::CharToDrive(gSessionPath[0],gTestDrive);
	test_KErrNone(r);

	r=RFs::DriveToChar(gTestDrive,gCh);
	test_KErrNone(r);

	TDriveInfo drv;
	r = TheFs.Drive(drv, gTestDrive);
	test_KErrNone(r);

	//-- print drive information
	PrintDrvInfo(TheFs, gTestDrive);

	// do not run the remainder of this test on RAM drive
	if (drv.iType == EMediaRam)
		{
		TestRAMDriveNotification();	// Test drive space reservations trigger disk space notifications
		test.Printf(_L("Main tests can't run on RAM drive %C:\n"), gSessionPath[0]);
		return;
		}

	if (Is_SimulatedSystemDrive(TheFs, gTestDrive))
		{
		test.Printf(_L("Skipping T_FALSESPACE on PlatSim/Emulator drive %C:\n"), gSessionPath[0]);
		return;
		}

    Test0();
	Test1();	// General test for new APIs
	Test2();	// Test to ensure drive and session reserve limits are not exceeded
	Test3();
	Test4();	// Test filling the drive and that each checked API fails
	Test5();
	Test6();
	Test7();
	TestForDEF142554();
	Test2();	// Run this test to check reserves are being cleared correctly

	TestFAT4G_Boundary();

	TurnAllocFailureOff();
	}
