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
// f32test\server\t_format.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"
#include "t_chlffs.h"

#include "f32_test_utils.h"

using namespace F32_Test_Utils;


GLREF_D TFileName gSessionPath;


void GenerateMediaChange()
	{
	TBuf<2> b;
	b.SetLength(2);
	b[0]=gSessionPath[0];
	b[1]=':';
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,b,EHighDensity,count);
	test_KErrNone(r);
	format.Close();
	}

RTest test(_L("T_FORMAT"));
RSemaphore gSleepThread;
TRequestStatus gThreadLogon;

static TInt gDrive=-1;
static const TInt KSectorSize=512;
static const TInt KHeapSize=0x200;

enum TTestCode{ETest3,ETest5};


//-------------------------------------------------------------------
TInt DoFormatSteps(RFormat& aFormat, TInt& aFmtCnt)
{
    TInt nRes = KErrNone;

    while(aFmtCnt)
    {
        nRes = aFormat.Next(aFmtCnt);
        if(nRes != KErrNone)
        {
            test.Printf(_L("RFormat::Next() failed! code:%d\n"), nRes);
            break;
        }
    }

    return nRes;
}

static void WaitForMedia()
//
// Wait until the media change is serviced
//
	{

	FOREVER
		{
		TInt r=TheFs.MkDir(_L("\\"));
		if (r!=KErrNotReady)
			break;
		User::After(100000);
		}
	}

static TInt ThreadEntryPoint(TAny* aTestCode)
//
// Thread entry point
//
	{

	RFs fs;
	TInt ret=fs.Connect();
	test_KErrNone(ret);
	ret=fs.SetSessionPath(gSessionPath);
	test_KErrNone(ret);
	TTestCode testCode=*(TTestCode*)&aTestCode;
	TInt count;
	RFormat format;
	switch (testCode)
		{
		case ETest3:
			{
			ret=format.Open(fs,gSessionPath,EQuickFormat,count);
			test_KErrNone(ret);
			
            ret = DoFormatSteps(format, count);
            test_KErrNone(ret);
	
            format.Close();
			break;
			}
		case ETest5:
			{
			ret=format.Open(fs,gSessionPath,EFullFormat,count);
			test_KErrNone(ret);
			gSleepThread.Signal();
			User::After(100000000);		
			break;
			}
		default:
			break;
		}
	return(KErrNone);
	}

//-------------------------------------------------------------------
static void CorruptCurrentDrive()
//
// Corrupt the current drive
//
	{
	test.Printf(_L("CorruptCurrentDrive() %c:"), 'A'+CurrentDrive());
    
	RRawDisk raw;
	TInt r=raw.Open(TheFs,CurrentDrive());
	test_KErrNone(r);
	if (!Is_Lffs(TheFs, gDrive))
		{
		TBuf8<KSectorSize> zeroBuf(KSectorSize);
		Mem::FillZ((TAny*)zeroBuf.Ptr(),zeroBuf.MaxSize());
		
        //-- for FAT32 we need to corrupt a backup BOOT sector as well,
        //-- otherwise it can be used and some tests will fail..
        const TInt KMaxSectors = 25; //-- how many sectors to corrupt
        for(TInt i=0; i<KMaxSectors; ++i)
            {
            r=raw.Write(i*KSectorSize, zeroBuf);
		    test_KErrNone(r);
            }
		}
	else
		{
		TBuf8<32> zeroBuf(32);
		for (TInt j=0;j<32;++j)
			zeroBuf[j]=(TUint8)j; //Not actuall zero buf for lffs
		// For LFFS, the media may not exhibit a contiguous region of sufficient length
		// to support a continuous sequence of writes. This is the case if the 
		// Control Mode Size is non-zero
		TInt cntlModeSize=GetLFFSControlModeSize();
		if(cntlModeSize==0)
			{
			//test.Printf(_L("CorruptCurrentDrive() - Control mode  size is zero\n"),r);
			for (TInt writePos=0;writePos<0x20200;writePos+=32)
				{
				r=raw.Write(writePos,zeroBuf);
				// The device driver most likely fails when writing a random
				// buffer due to read back checks. Since we're writing
				// aligned 32-byte blocks, we don't need to bother that much.
				// The device driver writes the block but fails when reading
				// it back.
				// test_KErrNone(r);
				}
			}
		else if(cntlModeSize>0)
			{
			//test.Printf(_L("CorruptCurrentDrive() - Control mode = 0x%x\n"),r);
			// For devices which have a non-zero control mode size, the writes may
			// require segmentation.
			TInt cmBase=0;
			TInt cmOffset=0;
			TInt bufOffset=0;
			TInt bytesWritten=0;
			TPtrC8 writeBuf;
			while(bytesWritten < 0x20200)
				{
				TInt bufLeft = 32 - bufOffset;	// 32 from size of zeroBuf
				TInt spaceLeft = cntlModeSize - cmOffset;
				TInt writeLen=(bufLeft>spaceLeft)? spaceLeft : bufLeft;
				writeBuf.Set(&(zeroBuf[bufOffset]),writeLen);
				TInt writePos = cmBase + cmOffset;
				r=raw.Write(writePos,writeBuf);
				bytesWritten += writeLen;
				if(bufLeft < spaceLeft)
					{
					bufOffset = 0;
					cmOffset += bufLeft;
					}
				else if(bufLeft == spaceLeft)
					{
					bufOffset = 0;
					cmOffset = 0;
					cmBase += (2*cntlModeSize);
					}
				else	
					{	// bufRemaining>spaceRemaining
					bufOffset += spaceLeft;
					cmOffset = 0;
					cmBase += (2*cntlModeSize);
					}
				}
			}
		else
			{
			// Negative value (error code) returned from GetLFFSControlModeSize()
			test.Printf(_L("CorruptCurrentDrive() - Control mode = %d (ERROR!) \n"),cntlModeSize);
			test(0);
			}
		}
	raw.Close();
	}


//-------------------------------------------------------------------
static void Test1()
//
// Format disk
//
	{
	
	test.Next(_L("Test EFullFormat"));
	TInt count;
	RFormat format;

	TInt r=format.Open(TheFs,gSessionPath,EFullFormat,count);
	test_KErrNone(r);

    r = DoFormatSteps(format, count);
    test_KErrNone(r);

	format.Close();
	
	TVolumeInfo volInfo;
	r=TheFs.Volume(volInfo);
	test_KErrNone(r);
	
	if (volInfo.iSize-volInfo.iFree!=0)
		{
		test.Printf(_L("Memory 'in use' after a full format = %ld\n"),(volInfo.iSize-volInfo.iFree));
		test.Printf(_L("volInfo.iSize = %ld\n"),volInfo.iSize);
		test.Printf(_L("volInfo.iFree = %ld\n"),volInfo.iFree);
		}
		
	test.Next(_L("Test EQuickFormat"));
	r=format.Open(TheFs,gSessionPath,EQuickFormat,count);
	test_KErrNone(r);

    r = DoFormatSteps(format, count);
    test_KErrNone(r);

	format.Close();

	r=TheFs.Volume(volInfo);
	test_KErrNone(r);
		
	if (volInfo.iSize-volInfo.iFree!=0)
		{
		test.Printf(_L("Memory 'in use' after a quick format = %ld\n"),(volInfo.iSize-volInfo.iFree));
		test.Printf(_L("volInfo.iSize = %ld\n"),volInfo.iSize);
		test.Printf(_L("volInfo.iFree = %ld\n"),volInfo.iFree);
		return;
		}
	
	}

//-------------------------------------------------------------------
static void Test2()
//
// Test access controls
//
	{
	
	test.Next(_L("Test disk cannot be formatted while a file is open"));
	RFile f;
	TInt r=f.Replace(TheFs,_L("BLARGME.BLARG"),EFileStream);
	test_KErrNone(r);

	TInt count;
	RFormat format;
	r=format.Open(TheFs,gSessionPath,EFullFormat,count);
	test_Value(r, r == KErrInUse);

	f.Close();
	r=format.Open(TheFs,gSessionPath,EFullFormat,count);
	test_KErrNone(r);
	format.Close();

	CheckFileExists(_L("BLARGME.BLARG"),KErrNone);
	}

//-------------------------------------------------------------------
static void Test3()
//
// Test notification
//
	{

	test.Next(_L("Test successful format triggers notifier"));
	MakeFile(_L("\\BLARG_BLARG_BLARG.BLG"));
	TRequestStatus reqStat;
	TheFs.NotifyChange(ENotifyEntry,reqStat);
	
	RThread clientThread;
	TInt r=clientThread.Create(_L("ClientThread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest3);
	test.Printf(_L("Created helper thread #1, res=%d\n"),r);	
	test_KErrNone(r);
	
	clientThread.Logon(gThreadLogon);
	clientThread.Resume();
	clientThread.Close();
	
	User::WaitForRequest(reqStat);
	test.Printf(_L("Notifier triggered #1, res=%d\n"),reqStat.Int());	

	User::WaitForRequest(gThreadLogon);
	test.Printf(_L("Helper thread exited #1, res=%d\n"),gThreadLogon.Int());	
    
	CheckFileExists(_L("BLARG_BLARG_BLARG.BLG"),KErrNotFound);
	MakeFile(_L("\\BLARG_BLARG_BLARG.BLG"));

	TheFs.NotifyChange(ENotifyAll,reqStat);
	r=clientThread.Create(_L("ClientThread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest3);
    test.Printf(_L("Created helper thread #2, res=%d\n"),r);	
	test_KErrNone(r);


	clientThread.Logon(gThreadLogon);
	clientThread.Resume();
	clientThread.Close();
	
	User::WaitForRequest(reqStat);
	test.Printf(_L("Notifier triggered #2, res=%d\n"),reqStat.Int());	

	User::WaitForRequest(gThreadLogon);
	test.Printf(_L("Helper thread exited #2, res=%d\n"),gThreadLogon.Int());	

	CheckFileExists(_L("BLARG_BLARG_BLARG.BLG"),KErrNotFound);
	}

//-------------------------------------------------------------------
static void Test4()
//
// Test partially completed formats
//
	{
    test.Next(_L("Test partially completed formats"));

    if(Is_Automounter(TheFs, gDrive))
        {//-- if we have automounter FS installed, formatting of the corrupted media is not straightforward
         //-- The automounter might not be able to decide which of the child file systems shall be used; This issue is covered in a specific test.
        test.Printf(_L("This step is skipped for Automounter File System\n"));	
        return;
        }

	MakeFile(_L("\\FORMAT\\DIR1\\BLARG_BLARG_BLARG.BLG"));
	TInt count;
	CorruptCurrentDrive();
	
    test.Printf(_L("Formatting the drive...\n"));	

	RFormat format;
	TInt r=format.Open(TheFs,gSessionPath,EFullFormat,count);
	test_KErrNone(r);

	while(count)
		{
		RDir dir;
		r=dir.Open(TheFs,_L("\\*.*"),KEntryAttNormal);
		test_Value(r, r == KErrInUse);
		r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();

	CheckFileExists(_L("\\FORMAT\\DIR1\\BLARG_BLARG_BLARG.BLG"),KErrPathNotFound);
	}

//-------------------------------------------------------------------
static void Test5()
//
// Test panic formatting thread
//
	{

    test.Next(_L("Test panic formatting thread"));

    if(Is_Automounter(TheFs, gDrive))
        {//-- if we have automounter FS installed, formatting of the corrupted media is not straightforward
         //-- The automounter might not be able to decide which of the child file systems shall be used; This issue is covered in a specific test.
        test.Printf(_L("This step is skipped for Automounter File System\n"));	
        return;
        }

	CorruptCurrentDrive();
    if (!(IsTestingLFFS() && GetDriveLFFS()==EDriveC)) // ??? Remove after ER5U
		{
//		UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change
		GenerateMediaChange();
		WaitForMedia();
		}

	gSleepThread.CreateLocal(0);
	RThread clientThread;
	TInt r=clientThread.Create(_L("ClientThread"),ThreadEntryPoint,0x4000,KHeapSize,KHeapSize,(TAny*)ETest5);
	test.Printf(_L("Created helper thread #1, res=%d\n"),r);	
	test_KErrNone(r);

    test.Printf(_L("Panicing formatting thread #1\n"));	
	clientThread.Resume();
	gSleepThread.Wait();
	test.Printf(_L("Panicing formatting thread #2\n"));	
	User::SetJustInTime(EFalse);
	clientThread.Panic(_L("Panic formatting thread"),KErrGeneral);
	User::SetJustInTime(ETrue);
	User::After(200000);	// to let panic take effect
    test.Printf(_L("Panicing formatting thread #3\n"));	

	RDir dir;
	r=dir.Open(TheFs,_L("\\*.*"),KEntryAttNormal);
//	if(IsTestingLFFS() && (r==KErrNone))
//	{
//		dir.Close();
//	}
//	else
//	{
	test_Value(r, r == KErrCorrupt);
//	}

    test.Printf(_L("Formatting the drive...\n"));	

	TInt count;
	RFormat format;
	r=format.Open(TheFs,gSessionPath,EQuickFormat,count);
	test_KErrNone(r);
	
    r = DoFormatSteps(format, count);
    test_KErrNone(r);
	
    format.Close();

	MakeFile(_L("BLARGOID.BLARG"));
	CheckFileExists(_L("BLARGOID.BLARG"),KErrNone);
	clientThread.Close();
	gSleepThread.Close();
	}		

//-------------------------------------------------------------------
static void Test6()
//
// Test ramdrive is shrunk after formatting
//
	{

	test.Next(_L("Test ramdrive shrinks after formatting"));
	TVolumeInfo volInfo;
	TInt r=TheFs.Volume(volInfo);
	test_KErrNone(r);
	if ((volInfo.iDrive.iMediaAtt&KMediaAttVariableSize)==0)
		return;

	TInt64 used=volInfo.iSize-volInfo.iFree;
	RFile f;
	r=f.Replace(TheFs,_L("BIGFILE.SIZE"),EFileRead|EFileWrite);
	test_KErrNone(r);
	f.SetSize(0x100000); // 1MB
	f.Close();

	r=TheFs.Volume(volInfo);
	test_KErrNone(r);
	TInt64 used2=volInfo.iSize-volInfo.iFree;
	test(used<used2);

	r=TheFs.Delete(_L("BIGFILE.SIZE"));
	test_KErrNone(r);
	r=TheFs.Volume(volInfo);
	test_KErrNone(r);
	used2=volInfo.iSize-volInfo.iFree;
	test(used==used2);

	r=f.Replace(TheFs,_L("BIGFILE.SIZE"),EFileRead|EFileWrite);
	test_KErrNone(r);
	f.SetSize(0x100000); // 1MB
	f.Close();

	r=TheFs.Volume(volInfo);
	test_KErrNone(r);
	used2=volInfo.iSize-volInfo.iFree;
	test(used<used2);

	TInt count;
	RFormat format;
	r=format.Open(TheFs,gSessionPath,EQuickFormat,count);
	test_KErrNone(r);
	
    r = DoFormatSteps(format, count);
    test_KErrNone(r);
	
    format.Close();

	r=TheFs.Volume(volInfo);
	test_KErrNone(r);
	used2=volInfo.iSize-volInfo.iFree;
	test(used>=used2);
	}

static void Test7()
//
// Generate media change before formatting.
//
	{

	test.Next(_L("Generate Media change before formatting"));

    if(Is_Automounter(TheFs, gDrive))
        {//-- if we have automounter FS installed, formatting of the corrupted media is not straightforward
         //-- The automounter might not be able to decide which of the child file systems shall be used; This issue is covered in a specific test.
        test.Printf(_L("This step is skipped for Automounter File System\n"));	
        return;
        }
	
    TVolumeInfo volInfo;
	TInt r=TheFs.Volume(volInfo);
	test_KErrNone(r);
	
    if (volInfo.iDrive.iMediaAtt&KMediaAttVariableSize)
		return; // Don't bother on internal disk
	
    if (Is_Lffs(TheFs, gDrive))
		return; // Don't bother on LFFS

	CorruptCurrentDrive();
//	UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change
	GenerateMediaChange();
	WaitForMedia();
	TInt count;
	RFormat format;
	r=format.Open(TheFs,gSessionPath,EQuickFormat,count);
	test_KErrNone(r);

    r = DoFormatSteps(format, count);
    test_KErrNone(r);

	format.Close();
	}

//-------------------------------------------------------------------
static void Test8()
//
// Test incomplete format
//
	{

	test.Next(_L("Test incomplete format"));

    if(Is_Automounter(TheFs, gDrive))
        {//-- if we have automounter FS installed, formatting of the corrupted media is not straightforward
         //-- The automounter might not be able to decide which of the child file systems shall be used; This issue is covered in a specific test.
        test.Printf(_L("This step is skipped for Automounter File System\n"));	
        return;
        }
	
    CorruptCurrentDrive();
    
    if (!(IsTestingLFFS() && GetDriveLFFS()==EDriveC)) // ??? Remove after ER5U
		{
//		UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change
		GenerateMediaChange();
		WaitForMedia();
		}
	
    TVolumeInfo volInfo;
	TInt r=TheFs.Volume(volInfo);
//	test_Value(r, r == KErrCorrupt);
	TInt count;
	RFormat format;
	r=format.Open(TheFs,gSessionPath,EQuickFormat,count);
	r=TheFs.Volume(volInfo);
	test_Value(r, r == KErrInUse);
	r=format.Next(count);
	test_KErrNone(r);
	TDriveList driveList;
	r=TheFs.DriveList(driveList);
	test_KErrNone(r);

    if(gDrive == EDriveC)
    {
		r=TheFs.Volume(volInfo, gDrive);
		test_Value(r, r == KErrInUse);
    } 
    else
    {
		r=TheFs.Volume(volInfo,EDriveC);
		test_KErrNone(r);

		r=TheFs.Volume(volInfo,gDrive);	
		test_Value(r, r == KErrInUse);

    	r=TheFs.Volume(volInfo,gDrive);	
		test_Value(r, r == KErrInUse);
    }

    
    format.Close();
	Format(CurrentDrive());
	}


//-------------------------------------------------------------------
/** 
    Test an API that allows force media formatting with the files or other objects opened on the volume
*/
void TestFormat_ForceDismount()
{
	test.Next(_L("Test format with forced media dismounting"));
   
    if(Is_Lffs(TheFs, gDrive))
    {//-- forced FS dismounting with files/directories opened damages LFFS structure for unknown reason.
     //-- this is a problem of LFFS, anyway, it is not supported.   
        test.Next(_L("This test can't be performed on LFFS, Skipping."));
        return;
    }

    TInt nRes;
    RFormat     format;
    TUint       fmtMode = EQuickFormat;
    TInt        fmtCnt;
    TBuf<10>    drivePath;
    drivePath.Format(_L("%C:\\"), gDrive+'A');
    

    RBuf8 buf8;
    RFile file1;
    RDir  dir;

    const TInt KBufLen = 128*K1KiloByte;
    nRes = buf8.CreateMax(KBufLen);
    test_KErrNone(nRes);

    _LIT(KFname, "\\file1");



    //---------------------------------------------------------------------------------
    //-- 1.1 open a file, try to format in normal mode; this shall fail with KErrInUse
    test.Printf(_L("Test normal format with normal opened objects\n"));
    nRes = file1.Replace(TheFs, KFname, EFileWrite);
    test_KErrNone(nRes);

    fmtMode = EQuickFormat;

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test_Value(nRes, nRes == KErrInUse);
    format.Close();

    buf8.SetLength(22);
    nRes = file1.Write(buf8);
    test_KErrNone(nRes);

    file1.Close();

    //-- 1.2 open a directory, try to format in normal mode; this shall fail with KErrInUse
	nRes = dir.Open(TheFs,_L("\\*.*"),KEntryAttNormal);
    test_KErrNone(nRes);

    fmtMode = EQuickFormat;
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test_Value(nRes, nRes == KErrInUse);
    format.Close();

    dir.Close();


    //---------------------------------------------------------------------------------
    //-- 2.1 forced quick formatting 
    test.Printf(_L("Test forced quick formatting\n"));
    nRes = file1.Replace(TheFs, KFname, EFileWrite); //-- open a file
    test_KErrNone(nRes);

    nRes = dir.Open(TheFs,_L("\\*.*"),KEntryAttNormal); //-- open a directory
    test_KErrNone(nRes);

    //-- this will mark the current Mount as "Dismounted" and will instantiate another CMountCB for formatting
    fmtMode = EQuickFormat | EForceFormat;

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test_KErrNone(nRes);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test_KErrNone(nRes);
    
    format.Close();

 
	nRes=TheFs.CheckDisk(gSessionPath);
	test_Value(nRes, nRes == KErrNone||nRes==KErrNotSupported);

    buf8.SetLength(22);
    nRes = file1.Write(buf8);
    test_Value(nRes, nRes == KErrDisMounted);
    file1.Close();  //-- this will make the previously "Dismounted" mount die.
    dir.Close();


    //---------------------------------------------------------------------------------
    //-- 2.2 forced full formatting 
    test.Printf(_L("Test forced full formatting\n"));
    nRes = file1.Replace(TheFs, KFname, EFileWrite);
    test_KErrNone(nRes);

    //-- this will mark the current Mount as "Dismounted" and will instantiate another CMountCB for formatting
    fmtMode = EFullFormat | EForceFormat; 

    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt); 
    test_KErrNone(nRes);
    
    nRes = DoFormatSteps(format, fmtCnt);
    test_KErrNone(nRes);
    
    format.Close();

	nRes=TheFs.CheckDisk(gSessionPath);
	test_Value(nRes, nRes == KErrNone||nRes==KErrNotSupported);

    buf8.SetLength(22);
    nRes = file1.Write(buf8);
    test_Value(nRes, nRes == KErrDisMounted);
    file1.Close();  //-- this will make the previously "Dismounted" mount die.

    test_Value(nRes, nRes == KErrDisMounted);
    file1.Close();  

    //---------------------------------------------------------------------------------
    //-- 4.1 check that forced formatting will succeed with dirty file cache
    test.Printf(_L("Test forced formatting will succeed with dirty file cache\n"));

    nRes = file1.Replace(TheFs, KFname, EFileWrite | EFileWriteBuffered); //-- enable write caching
    test_KErrNone(nRes);
    
    buf8.SetLength(KBufLen);
    nRes = file1.Write(buf8); //-- this will hopefully get via file write cache
    test_KErrNone(nRes);
  
    fmtMode = EQuickFormat | EForceFormat; 
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt); 
    test_KErrNone(nRes);
    format.Close();

    nRes = file1.Write(buf8);
    test_Value(nRes, nRes == KErrDisMounted);
    file1.Close();  


    //---------------------------------------------------------------------------------
    
    test.Printf(_L("Test forced formatting with disk access objects opened\n"));

    //-- 5.1 check that forced formatting will fail when there are "disk access" objects opened RFormat
    RFormat     format1;

    nRes = format1.Open(TheFs, drivePath, fmtMode, fmtCnt);
    test_KErrNone(nRes);
    
    fmtMode = EQuickFormat | EForceFormat; 
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt); 
    test_Value(nRes, nRes == KErrInUse);
    format.Close();

    format1.Close();

    //-- 5.1 check that forced formatting will fail when there are "disk access" objects opened RRawDisk
    RRawDisk    rawDisk;
    nRes = rawDisk.Open(TheFs, gDrive);
    test_KErrNone(nRes);

    fmtMode = EQuickFormat | EForceFormat; 
    nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt); 
    test_Value(nRes, nRes == KErrInUse);
    format.Close();

    rawDisk.Close();


    //---------------------------------------------------------------------------------
    //-- 6. Try forced formatting with clamped files, this shall fail with KErrInuse
    test.Printf(_L("Test forced formatting and clamps on the volume\n"));

    nRes = file1.Replace(TheFs, KFname, EFileWrite);
    test_KErrNone(nRes);
    
    buf8.SetLength(KBufLen);
    nRes = file1.Write(buf8); //-- this will hopefully get via file write cache
    test_KErrNone(nRes);
    file1.Flush();

    //-- Clamp file
    RFileClamp handle;
    
    nRes=handle.Clamp(file1);
    if(nRes != KErrNone)
    {
        test.Printf(_L("file clamps on this drive are not supported\n"));
    }
    else
    {
        fmtMode = EQuickFormat | EForceFormat; 
        nRes = format.Open(TheFs, drivePath, fmtMode, fmtCnt); 
        test_Value(nRes, nRes == KErrInUse);
        format.Close();
    }
    
    handle.Close(TheFs);

    file1.Close();

    buf8.Close();
}


void CallTestsL()
//
// Call tests that may leave
//
	{

    TInt r;
    r = TheFs.CharToDrive(gDriveToTest, gDrive);
    test_KErrNone(r);

    //-- set up console output 
    F32_Test_Utils::SetConsole(test.Console()); 
    
    TInt nRes=TheFs.CharToDrive(gDriveToTest, gDrive);
    test_KErrNone(nRes);
    
    PrintDrvInfo(TheFs, gDrive);

    if(Is_SimulatedSystemDrive(TheFs, gDrive))
    	{
		test.Printf(_L("Skipping T_FORMAT on PlatSim/Emulator drive %C:\n"), gSessionPath[0]);
		return;
    	}


	SetSessionPath(_L("\\"));

	Test1();
	Test2();
	Test3();
	Test4();
	Test5();
	Test6();
	Test7();
	Test8();
    TestFormat_ForceDismount();

	r=TheFs.CheckDisk(gSessionPath);
	test_Value(r, r == KErrNone||r==KErrNotSupported);
	}


