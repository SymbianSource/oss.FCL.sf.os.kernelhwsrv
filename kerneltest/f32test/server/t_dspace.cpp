// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_dspace.cpp
//
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include "t_server.h"
#include "t_chlffs.h"


/* This tests disk space notification. Using RFs::NotifyDiskSpace a client can request
to be notified if the free disk space crosses a specified threshold. This test requires
a card to be present in d: */

GLDEF_D RTest test(_L("T_DSPACE"));

const TInt KMaxBufSize=512;
#if defined(__WINS__)
TInt KFileSize1=2048;
TInt KFileSize2=4096;
TInt KFileSize3=8192;
#else
TInt KFileSize1=512;
TInt KFileSize2=1024;
TInt KFileSize3=4096;
#endif

const TInt KHeapSize=0x2000;
const TInt KStackSize=0x4000;

TInt gMinFileSize;

TBool LffsDrive = EFalse;

TBuf8<KMaxBufSize> TheBuffer;
TInt64 TheDiskSize;
TInt RemovableDrive;
TBuf<4> RemovableDriveBuf=_L("?:\\");

_LIT(KTestFile1, "\\F32-TST\\testfile1");
_LIT(KTestFile2, "\\F32-TST\\testFile2");
_LIT(KTestDir1, "\\F32-TST\\testDir1\\");
_LIT(KTestDir2, "\\F32-TST\\testDir2\\");
_LIT(KFileFiller, "\\F32-TST\\fileFiller");

// functions that may cause change in free disk space
// not all of them of tested since some require knowledge of file system
// to ensure change in free disk space
enum TThreadTask
	{
	ETaskSetVolume,
	ETaskMkDir,
	ETaskRmDir,
	ETaskDelete,		// test
	ETaskRename,
	ETaskReplace,		// test
	ETaskFileCreate,
	ETaskFileReplace,	// test
	ETaskFileTemp,
	ETaskFileWrite,		// test
	ETaskFileWrite4KB,
	ETaskFileWrite64KB,
	ETaskFileSetSize,	// test
	ETaskFileRename,
	ETaskNoChange1,
	ETaskNoChange2,
	ETaskFileCreateLffs,// test
	ETaskSpin
	};


LOCAL_C TBool IsWinsCDrive(TInt aDrive)
//
//
//
	{
#if defined(__WINS__)
	if(aDrive==KDefaultDrive)
		return(gSessionPath[0]==(TText)'C');
	else
		return(aDrive==EDriveC);
#else
	return(EFalse);
#endif
	}

LOCAL_C TInt64 FreeDiskSpace(TInt aDrive)
//
//
//
	{
	TVolumeInfo v;
	TInt r=TheFs.Volume(v,aDrive);
	test_KErrNone(r);
	return(v.iFree);
	}

LOCAL_C TInt64 DiskSize(TInt aDrive)
//
//
//
	{
	TVolumeInfo v;
	TInt r=TheFs.Volume(v,aDrive);
	test_KErrNone(r);
	return(v.iSize);
	}

// MinimumFileSize() -
// Deduces the minimum space occupied by a file by creating a file of one byte
// in length. This should equal the cluster size on FAT volumes.
//
LOCAL_C TInt MinimumFileSize(TInt aDrive)
	{
	TInt r = TheFs.Delete(KTestFile1);
	test_Value(r, r == KErrNone || r==KErrNotFound);

	TInt64 freeSpace = FreeDiskSpace(aDrive);

	RFile file;


	r=file.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
	test_KErrNone(r);

	r = file.Write(TheBuffer,1);
	test_KErrNone(r);
	file.Close();
	TInt64 newSpace = FreeDiskSpace(aDrive);

	r = TheFs.Delete(KTestFile1);
	test_KErrNone(r);


	TInt64 minFileSize = freeSpace - newSpace;
	test (minFileSize >= 0);
	minFileSize = Max(minFileSize, 512);
	test (minFileSize < KMaxTInt);

	TInt minFileSizeLow = I64LOW(minFileSize);

	RDebug::Print(_L("minFileSize %u"), minFileSizeLow);

#if defined(__WINS__)
	KFileSize1 = minFileSizeLow << 2;	// 512 * 2^2 = 512 * 4 = 2048;
	KFileSize2 = minFileSizeLow << 3;	// 512 * 2^3 = 512 * 8 = 4096;
	KFileSize3 = minFileSizeLow << 4;	// 512 * 2^4 = 512 * 16 = 8192;
#else
	KFileSize1 = minFileSizeLow;		// 512;
	KFileSize2 = minFileSizeLow << 1;	// 512 * 2^1 = 512 * 2 = 1024;
	KFileSize3 = minFileSizeLow << 3;	// 512 * 2^3 = 512 * 8 = 4096;
#endif


	return (TInt) minFileSizeLow;
	}

LOCAL_C void Initialise()
//
// do any initialisation before starting tests
//
	{
	if(TheBuffer.Length()!=KMaxBufSize)
		{
		TheBuffer.SetLength(KMaxBufSize);
		Mem::Fill((void*)TheBuffer.Ptr(),KMaxBufSize,0xab);
		}
#if defined(__WINS__)
	RemovableDrive=EDriveX;
#else
	TDriveList drvList;
	if(KErrNone == TheFs.DriveList(drvList))
		{
		TInt i;
		//should be successful, otherwise it means a system w/o any drive!!!
		for(i=0;i<KMaxDrives;i++)
			{
			TDriveInfo driveInfo;
			if((drvList[i] != 0)
				&& (KErrNone == TheFs.Drive(driveInfo, i))
				&& (driveInfo.iType == EMediaHardDisk))
				{
				RemovableDrive = i;
				test.Printf(_L("RemovableDrive = %d\n"),RemovableDrive);
				break;
				}
			}
		if(i == KMaxDrives)
			{
			test.Printf(_L("No Removable media found! Testing discontinued.\n"));
			User::Exit(KErrNone);
			}
		}
	else
		{
		test.Printf(_L("No Drive found! Testing discontinued.\n"));
		User::Exit(KErrNone);
		}
#endif

	test.Printf(_L("inside init++++++++++++++++++++++++++>\n\n\n"));
	test.Printf(_L("RemovableDrive = %d\n"),RemovableDrive);
	// initialise removable drive descriptor
	TChar c;
	TInt r=RFs::DriveToChar(RemovableDrive,c);
	test_KErrNone(r);
	RemovableDriveBuf[0]=(TText)c;

	if( !LffsDrive )
		{
		// and create the default directory
		TFileName d=gSessionPath;
		d[0]=RemovableDriveBuf[0];
		MakeDir(d);
		}

	// better format the default drive as long as not WINS c drive
	TInt drive;
	r= RFs::CharToDrive(gSessionPath[0],drive);
	test_KErrNone(r);
#if defined(__WINS__)
	if(drive!=EDriveC)
		Format(drive);
#else
	Format(drive);
	// test not run on c: drive but does use it
	Format(EDriveC);
#endif
	TheDiskSize=DiskSize(KDefaultDrive);
	// and set the default directory
	r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);

	r=TheFs.Delete(KFileFiller);
	test_Value(r, r == KErrNone || r==KErrNotFound);
	r=TheFs.Delete(KTestFile1);
	test_Value(r, r == KErrNone || r==KErrNotFound);
	r=TheFs.Delete(KTestFile2);
	test_Value(r, r == KErrNone || r==KErrNotFound);
	r=TheFs.RmDir(KTestDir1);
	test_Value(r, r == KErrNone || r==KErrNotFound);
	r=TheFs.RmDir(KTestDir2);
	test_Value(r, r == KErrNone || r==KErrNotFound);

	gMinFileSize = MinimumFileSize(drive);
	}

LOCAL_C TInt64 FillDisk(RFile& aFile,TInt64 aNewSpace,TInt aDrive)
//
// fill a file until free disk space equals aFreeSpace
//
	{
	TInt64 space=FreeDiskSpace(aDrive);
	RDebug::Print(_L("Filling drive till %lu bytes left, current freespace is %lu."), aNewSpace, space);
	
	test(space>aNewSpace);
	while(space>aNewSpace)
		{
		TInt s=Min(KMaxBufSize, I64INT(space-aNewSpace));
		TInt r=aFile.Write(TheBuffer,s);
		if( !LffsDrive )
			{
			test_KErrNone(r);
			}
		else
			{
			//
			// LFFS is less predictable than a normal drive because of the logging
			// and metadata arrangement
			//
			test( (KErrNone==r) || (KErrDiskFull==r) );
			if( KErrDiskFull == r )
				{
				// shrink the file back down again to give the requested free space
				TInt fileSize;
				r=aFile.Size( fileSize );
				test( KErrNone == r );
				test( TInt64(fileSize) > aNewSpace );
				fileSize -= I64LOW(aNewSpace);
				r=aFile.SetSize( fileSize );
				test( KErrNone == r );

				space=FreeDiskSpace(aDrive);
				while( space < aNewSpace )
					{
					fileSize -= I64LOW(aNewSpace - space);
					test( fileSize > 0 );
					r=aFile.SetSize( fileSize );
					test( KErrNone == r );
					space=FreeDiskSpace(aDrive);
					}
				break;
				}
			}

		space=FreeDiskSpace(aDrive);
		}
	return(space);
	}

LOCAL_C void WriteToFile(RFile& aFile,TInt aSize)
//
//
//
	{
	while(aSize>0)
		{
		TInt s=Min(KMaxBufSize,aSize);
		TInt r=aFile.Write(TheBuffer,s);
		aSize-=s;

		// Flush if write caching enabled to ensure we get disk space notifications
		if ((gDriveCacheFlags & EFileCacheWriteOn) && (r == KErrNone))
			r = aFile.Flush();

		if( !LffsDrive )
			{
			test_KErrNone(r);
			}
		else
			{
			// we can't accurately predict the amount of data we can actually get
			// on an LFFS drive, so it's posible we could exceed the available
			// space even though we are writing less that the reported free space
			test( (KErrNone==r) || (KErrDiskFull==r) );
			if( KErrDiskFull == r )
				{
				break;	// just stop
				}
			}
		}
	}


LOCAL_C void CleanupForThread(TInt aTask)
//
//
//
	{
	TInt r;
	switch(aTask)
		{
		case ETaskMkDir:
			r=TheFs.RmDir(KTestDir1);
			test_KErrNone(r);
			break;
		case ETaskRmDir: break;
		case ETaskDelete: break;
		case ETaskReplace:
			r=TheFs.Delete(KTestFile2);
			test_KErrNone(r);
			break;
		case ETaskFileReplace:
			r=TheFs.Delete(KTestFile1);
			test_KErrNone(r);
			break;
		case ETaskFileWrite:
		case ETaskFileWrite4KB:
		case ETaskFileWrite64KB:
		case ETaskFileSetSize:
		case ETaskFileCreateLffs:
		case ETaskNoChange1:
		case ETaskNoChange2:
			r=TheFs.Delete(KTestFile1);
			if(r!=KErrNone)
				{
				test.Printf(_L("r=%d"),r);
				test(EFalse);
				}
			break;
		case ETaskSpin:
		default:break;
		}
	}

LOCAL_C void InitialiseForThread(TInt aTask)
//
//
//
	{
	TInt r;
	RFile file,file2;
	switch(aTask)
		{
		case ETaskMkDir:	break;
		case ETaskRmDir:
			r=TheFs.MkDir(KTestDir1);
			test_KErrNone(r);
			break;
		case ETaskDelete:
			r=file.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			if( !LffsDrive )
				{
				r=file.SetSize(KFileSize1);
				test_KErrNone(r);
				}
			else
				{
				// LFFS supports sparse files, so we have to write real data
				// into the file to ensure that it uses up disk space
				WriteToFile( file, KFileSize1 );
				}
			file.Close();
			break;
		case ETaskReplace:
			r=file.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			if( !LffsDrive )
				{
				r=file.SetSize(KFileSize1);
				test_KErrNone(r);
				}
			else
				{
				WriteToFile( file, KFileSize2 );
				}
			file.Close();
			r=file2.Create(TheFs,KTestFile2,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			if( !LffsDrive )
				{
				r=file2.SetSize(KFileSize3);
				test_KErrNone(r);
				}
			else
				{
				WriteToFile( file2, gMinFileSize << 4);	// 512 * 16 = 8K
				}
			file2.Close();
			break;
		case ETaskFileReplace:
			r=file.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			if( !LffsDrive )
				{
				r=file.SetSize(KFileSize3*2);
				}
			else
				{
				WriteToFile( file, KFileSize3 );
				}
			test_KErrNone(r);
			file.Close();
			break;
		case ETaskFileWrite:
		case ETaskFileWrite4KB:
		case ETaskFileWrite64KB:
		case ETaskFileSetSize:
			r=file.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			file.Close();
			break;
		case ETaskNoChange1:
			r=file.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			if( !LffsDrive )
				{
				r=file.SetSize(KFileSize1);
				test_KErrNone(r);
				}
			else
				{
				WriteToFile( file, KFileSize1 );
				}
			file.Close();
			break;
		case ETaskNoChange2:
			r=file.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			file.Close();
			break;
		case ETaskFileCreateLffs:
			r = TheFs.Delete(KTestFile1);
			break;
		case ETaskSpin:
		default:break;
		}
	}

LOCAL_C TInt ThreadFunction(TAny* aThreadTask)
//
//
//
	{
	RTest test(_L("T_DSPACE_ThreadFunction"));
	RFs fs;
	TInt r=fs.Connect();
	test_KErrNone(r);
	r=fs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	TThreadTask task=*(TThreadTask*)&aThreadTask;
	RFile file;
	switch(task)
		{
		case ETaskMkDir:
			r=fs.MkDir(KTestDir1);
			test_KErrNone(r);
			break;
		case ETaskRmDir:
			r=fs.RmDir(KTestDir1);
			test_KErrNone(r);
			break;
		case ETaskDelete:
			r=fs.Delete(KTestFile1);
			test_KErrNone(r);
			break;
		case ETaskReplace:
			r=fs.Replace(KTestFile1,KTestFile2);
			test_KErrNone(r);
			break;
		case ETaskFileReplace:
			r=file.Replace(fs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			file.Close();
			break;
		case ETaskFileWrite:
			r=file.Open(fs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
#if defined(__WINS__)
			WriteToFile( file, gMinFileSize << 4);	// 512 * 16 = 8K
#else
			WriteToFile( file, gMinFileSize << 1);	// 512 * 2 = 1K
#endif
			file.Close();
			break;
		case ETaskFileWrite4KB:
			r=file.Open(fs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			WriteToFile(file,gMinFileSize << 3);	// 512 * 2^3 = 512 * 8 = 4K
			file.Close();
			break;
		case ETaskFileWrite64KB:
			r=file.Open(fs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			WriteToFile(file,gMinFileSize<<7);	// 512 * 2^7 = 512 * 128 = 64K
			file.Close();
			break;
		case ETaskFileSetSize:
			r=file.Open(fs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			r=file.SetSize(KFileSize3);
			file.Close();
			break;
		case ETaskFileCreateLffs:
			r=file.Create(fs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			file.Close();
			break;
		case ETaskNoChange1:
			{
			r=file.Open(fs,KTestFile1,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			TTime time;
			time.HomeTime();
			r=file.SetModified(time);
			test_KErrNone(r);
			file.Close();
			break;
			}
		case ETaskNoChange2:
			{
			TEntry e;
			r=fs.Entry(KTestFile1,e);
			test_KErrNone(r);
			break;
			}
		case ETaskSpin:
			for(;;) {};

		default:break;
		}
	fs.Close();
	return(KErrNone);
	}


void TestCancellation()
//
// test error disk space notification requests can be cancelled
//
	{
	test.Next(_L("test disk space cancellation"));
	const TInt ThresholdSize=500;
	// test a cancellation
	// Up the priority of this thread so that we can cancel the request before the drive thread
	// runs, to test whether cancelling still works.
	RThread().SetPriority(EPriorityRealTime);
	TRequestStatus stat1;
	TheFs.NotifyDiskSpace(ThresholdSize,KDefaultDrive,stat1);
	test(stat1==KRequestPending);
	TheFs.NotifyDiskSpaceCancel(stat1);
	test(stat1==KErrCancel);
	RThread().SetPriority(EPriorityNormal);
	// test no affect if already cancelled
	stat1=KErrNone;
	TheFs.NotifyDiskSpaceCancel(stat1);
	test(stat1==KErrNone);
	// set up two requests, cancel 1
	TRequestStatus stat2;
	TheFs.NotifyDiskSpace(ThresholdSize,KDefaultDrive,stat1);
	TheFs.NotifyDiskSpace(ThresholdSize,KDefaultDrive,stat2);
	test(stat1==KRequestPending && stat2==KRequestPending);
	TheFs.NotifyDiskSpaceCancel(stat2);
	test(stat1==KRequestPending && stat2==KErrCancel);
	TheFs.NotifyDiskSpaceCancel(stat1);
	test(stat1==KErrCancel);

	if( !LffsDrive)
		{
		// now repeat with c: and removable drive
		TheFs.NotifyDiskSpace(ThresholdSize,EDriveC,stat1);
		TheFs.NotifyDiskSpace(ThresholdSize,RemovableDrive,stat2);
		test(stat1==KRequestPending && stat2==KRequestPending);
		TheFs.NotifyDiskSpaceCancel(stat1);
		test(stat2==KRequestPending && stat1==KErrCancel);
		TheFs.NotifyDiskSpaceCancel(stat2);
		test(stat2==KErrCancel);
		}
	}

void TestErrorConditions()
//
// test disk space notification requests return correct error value
//
	{
	test.Next(_L("test error conditions"));
	// attempt to set up disk space notification with a threshold of zero
	TRequestStatus status;
	TheFs.NotifyDiskSpace(0,KDefaultDrive,status);
	test(status==KErrArgument);
	// test on an empty drive
	TheFs.NotifyDiskSpace(100,EDriveO,status);
	test(status==KErrNotReady);
	// test on a drive out of rance
	TheFs.NotifyDiskSpace(100,27,status);
	test(status==KErrBadName);
	// new setup with threshold of one
	TheFs.NotifyDiskSpace(1,KDefaultDrive,status);
	test(status==KRequestPending);
	TheFs.NotifyDiskSpaceCancel(status);
	test(status==KErrCancel);
	// and with a threshold > disk size
	TheFs.NotifyDiskSpace(TheDiskSize+10,KDefaultDrive,status);
	test(status==KErrArgument);
	// now with a size of max size -1
	TheFs.NotifyDiskSpace(TheDiskSize-1,KDefaultDrive,status);
	test(status==KRequestPending);
	TheFs.NotifyDiskSpaceCancel(status);
	test(status==KErrCancel);
	// set up mutiple requests and cancel one
	TRequestStatus status2,status3;
	TheFs.NotifyDiskSpace(TheDiskSize-10,KDefaultDrive,status);
	TheFs.NotifyDiskSpace(TheDiskSize-10,KDefaultDrive,status2);
	TheFs.NotifyDiskSpace(TheDiskSize-10,KDefaultDrive,status3);
	test(status==KRequestPending&&status2==KRequestPending&&status3==KRequestPending);
	TheFs.NotifyDiskSpaceCancel(status3);
	test(status==KRequestPending&&status2==KRequestPending&&status3==KErrCancel);
	// cancel the remaining ones
	TheFs.NotifyDiskSpaceCancel();
	test(status==KErrCancel&&status2==KErrCancel&&status3==KErrCancel);
	}

TInt GenerateMediaChange()
	{
	RLocalDrive d;
	TBool flag=EFalse;
	//Find the local drive number corresponding to removabledrive
	TMediaSerialNumber serialNum;
	TInt r = TheFs.GetMediaSerialNumber(serialNum, RemovableDrive);
	if(r!= KErrNone)
		return r;

	TInt len = serialNum.Length();
	test.Printf(_L("Serial number (len %d) :"), len);

	TInt localDriveNum = -1;
	for (TInt n=0; n<KMaxLocalDrives; n++)
		{
		r = d.Connect(n, flag);
		if(r != KErrNone)
			{
			test.Printf(_L("drive %d: TBusLocalDrive::Connect() failed %d\n"), n, r);
			continue;
			}

	    TLocalDriveCapsV5Buf capsBuf;
	    TLocalDriveCapsV5& caps = capsBuf();
		r = d.Caps(capsBuf);
		if(r != KErrNone)
			{
			test.Printf(_L("drive %d: TBusLocalDrive::Caps() failed %d\n"), n, r);
			continue;
			}


		TPtrC8 localSerialNum(caps.iSerialNum, caps.iSerialNumLength);
		if (serialNum.Compare(localSerialNum) == 0)
			{
				localDriveNum = n;
				d.Close();
				break;
			}

		d.Close();
		}
	r =d.Connect(localDriveNum,flag);
	if (r!=KErrNone)
		return r;
	d.ForceMediaChange();
	d.Close();
	return KErrNone;
	}

void TestDiskNotify()
//
// test functions that can result in disk change notification
// format,scandrive, media change
//
	{
    test.Next(_L("test Disk Notify"));
	// make default directory
	_LIT(defaultDir,"C:\\F32-TST\\");
	TInt r=TheFs.MkDirAll(defaultDir);
	test_Value(r, r == KErrNone||r==KErrAlreadyExists);
	// create the filler file
	RFile file;
	TFileName fileName=_L("C:");
	fileName+=KFileFiller;
	r=file.Create(TheFs,fileName,EFileShareAny|EFileWrite);
	test_KErrNone(r);
	TInt64 free=FreeDiskSpace(EDriveC);
	// use up 16KB
	FillDisk(file,free-16384,EDriveC);

	// test formatting notifies clients on only specific drive
	test.Next(_L("test formatting"));
	TRequestStatus stat1, stat2;
	TInt64 freeC=FreeDiskSpace(EDriveC);
	TInt64 freeD=FreeDiskSpace(RemovableDrive);
	TheFs.NotifyDiskSpace(freeC+1024,EDriveC,stat1);
	TheFs.NotifyDiskSpace(freeD-1024,RemovableDrive,stat2);
	test(stat1==KRequestPending && stat2==KRequestPending);
	RFormat f;
	TInt count;
	r=f.Open(TheFs,RemovableDriveBuf,EQuickFormat,count);
	test_KErrNone(r);
	while(count)
		{
		r=f.Next(count);
		test_KErrNone(r);
		}
	f.Close();
	User::After(1000000);
	User::WaitForRequest(stat2);
	test(stat1==KRequestPending && stat2==KErrNone);
	TheFs.NotifyDiskSpaceCancel(stat1);
	test(stat1==KErrCancel);

	// and create the test directory for the removable drive
	TFileName fName=_L("");
	fName+=RemovableDriveBuf;
	fName+=_L("F32-TST\\");
	r=TheFs.MkDirAll(fName);
	test_KErrNone(r);

	// test that a media change notifies clients on all drives
	test.Next(_L("media change"));
	freeC=FreeDiskSpace(EDriveC);
	freeD=FreeDiskSpace(RemovableDrive);
	test.Printf(_L("free space on drive %d = 0x%x\n"),EDriveC,freeC);
	test.Printf(_L("free space on drive %d = 0x%x\n"),RemovableDrive,freeD);
	TheFs.NotifyDiskSpace(freeC+1024,EDriveC,stat1);
	TheFs.NotifyDiskSpace(freeD-1024,RemovableDrive,stat2);
	test(stat1==KRequestPending && stat2==KRequestPending);
//	UserSvr::ForceRemountMedia(ERemovableMedia0);
	r = GenerateMediaChange();
	if(r == KErrNone)
		{
		User::After(1000000);
		User::WaitForRequest(stat2);
		test(stat1==KRequestPending && stat2==KErrNone);
		TheFs.NotifyDiskSpaceCancel(stat1);
		test(stat1==KErrCancel);
		}
	else
		{
		test.Printf(_L("media change not supported, skipping this step\n"));
		TheFs.NotifyDiskSpaceCancel(stat1);
		test(stat1 == KErrCancel);
		TheFs.NotifyDiskSpaceCancel(stat2);
		test(stat2 == KErrCancel);
		}

	// test that scandrive notifies clients on only specific drives
	test.Next(_L("scandrive"));
	// first test that scandrive does not find any problems on the removable media
	r=TheFs.ScanDrive(RemovableDriveBuf);
	test_KErrNone(r);
	// now set up disk space notification
	freeC=FreeDiskSpace(EDriveC);
	freeD=FreeDiskSpace(RemovableDrive);
	test.Printf(_L("free space on drive %d = 0x%x\n"),EDriveC,freeC);
	test.Printf(_L("free space on drive %d = 0x%x\n"),RemovableDrive,freeD);
	TheFs.NotifyDiskSpace(freeC+8192,EDriveC,stat1);
	TheFs.NotifyDiskSpace(freeD-8192,RemovableDrive,stat2);
	test(stat1==KRequestPending && stat2==KRequestPending);
	r=TheFs.ScanDrive(RemovableDriveBuf);
	test_KErrNone(r);
	User::After(1000000);
	User::WaitForRequest(stat2);
	test(stat1==KRequestPending && stat2==KErrNone);
	TheFs.NotifyDiskSpaceCancel(stat1);
	test(stat1==KErrCancel);

	file.Close();
	r=TheFs.Delete(fileName);
	test_KErrNone(r);
	if(gSessionPath[0]!=(TText)'C')
		{
		r=TheFs.RmDir(defaultDir);
		test_Value(r, r == KErrNone||r==KErrInUse);
		}

	}

void TestFunctions()
//
// test some of the functions that may result in a change in free disk space
// not testing all functions that may result in free disk space change since
// change is dependent on the file system
//
	{
	test.Next(_L("test disk space functions"));
	// create the filler file
	RFile file;
	TInt r=file.Create(TheFs,KFileFiller,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	test_KErrNone(r);
	TInt64 newSpace = FreeDiskSpace(KDefaultDrive)-8192;
	FillDisk(file,newSpace,KDefaultDrive);

	// check file write
	test.Next(_L("check RFile:Write"));
	TThreadTask task=ETaskFileWrite;
	InitialiseForThread(task);
	TInt64 free=FreeDiskSpace(KDefaultDrive);
	TRequestStatus stat1;
	TInt64 threshold=free-200;
	TheFs.NotifyDiskSpace(threshold,KDefaultDrive,stat1);
	test(stat1==KRequestPending);
	RThread thread;
	r=thread.Create(_L("thread1"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	thread.Resume();
	User::WaitForRequest(stat1);
	test(stat1==KErrNone);
	free=FreeDiskSpace(KDefaultDrive);
	test(free<threshold);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);

	// check file set size
	// setting file size on LFFS only uses a small amount of disk space for metadata
	// so we skip this test for an LFFS drive
	if( !LffsDrive )
		{
		test.Next(_L("check RFile:SetSize"));
		task=ETaskFileSetSize;
		InitialiseForThread(task);
		free=FreeDiskSpace(KDefaultDrive);
		threshold=free-100;
		TheFs.NotifyDiskSpace(threshold,KDefaultDrive,stat1);
		test(stat1==KRequestPending);
		r=thread.Create(_L("thread2"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
		test_KErrNone(r);
		TRequestStatus deathStat;
		thread.Logon( deathStat );
		thread.Resume();
		User::WaitForRequest(stat1);
		test(stat1==KErrNone);
		free=FreeDiskSpace(KDefaultDrive);
		test(free<threshold);
		User::WaitForRequest( deathStat );
		thread.Close();
		CleanupForThread(task);
		}

	// check disk space notification does not occur when threshold not crossed
	TInt64 newFree;
	test.Next(_L("check RFile:SetSize with wrong threshold"));

	User::After(1000000);	//put in due to thread latency

	task=ETaskFileSetSize;
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	threshold=free+100;
	TheFs.NotifyDiskSpace(threshold,KDefaultDrive,stat1);
	test(stat1==KRequestPending);
	r=thread.Create(_L("thread3"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	thread.Logon( deathStat );
	thread.Resume();

	User::After(1000000);

	test(stat1==KRequestPending);
	newFree=FreeDiskSpace(KDefaultDrive);
/*
	test.Printf(_L("threshold = %d and %d"),threshold.High(), threshold.Low());
	test.Printf(_L("free = %d and %d"),free.High(), free.Low());
	test.Printf(_L("newFree = %d and %d"),newFree.High(), newFree.Low());
*/
	if(!LffsDrive)
		test(free<threshold && newFree<free);
	else
		test(free<threshold);		//changing file size on lffs does not do anything

	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);
	TheFs.NotifyDiskSpaceCancel(stat1);
	test(stat1==KErrCancel);

	// check for file delete
	test.Next(_L("check RFs::Delete"));
	task=ETaskDelete;
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	threshold=free+300;
	TheFs.NotifyDiskSpace(threshold,KDefaultDrive,stat1);
	test(stat1==KRequestPending);
	r=thread.Create(_L("thread4"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	thread.Logon( deathStat );
	thread.Resume();
	User::WaitForRequest(stat1);
	test(stat1==KErrNone);
	free=FreeDiskSpace(KDefaultDrive);
	test(free>threshold);
	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);

	// check for replace with threshold too high
	test.Next(_L("check RFs::Replace with threshold too high"));

    if( LffsDrive )
	{
	    test.Printf( _L("Skipped.... test isn't consistent on LFFS drive\n") );
	}
    else
	{
        task=ETaskReplace;
	    InitialiseForThread(task);
	    free=FreeDiskSpace(KDefaultDrive);
    #if defined(__WINS__)
	    threshold=free + gMinFileSize * 16 + 2048;		// 512 * 16 + 2K = 10K
    #else
	    if(LffsDrive )
		    threshold=free + (gMinFileSize << 4) + 2048;	// 512 * 16 + 2K = 10K
	    else
		    threshold=free + gMinFileSize * 9 + 392;	// 512 * 9 + 392 = 5000;
    #endif

	    TheFs.NotifyDiskSpace(threshold,KDefaultDrive,stat1);
	    test(stat1==KRequestPending);

	    r=thread.Create(_L("thread5"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	    test_KErrNone(r);
	    thread.Logon( deathStat );
	    thread.Resume();

	    User::After(1000000);
	    test(stat1==KRequestPending);
	    newFree=FreeDiskSpace(KDefaultDrive);
	    test(newFree<threshold && free<newFree);
	    TheFs.NotifyDiskSpaceCancel();
	    test(stat1==KErrCancel);
	    User::WaitForRequest( deathStat );
	    thread.Close();
	    CleanupForThread(task);


	    // check for replace
	    test.Next(_L("check RFs:Replace"));
	    task=ETaskReplace;
	    InitialiseForThread(task);
	    free=FreeDiskSpace(KDefaultDrive);
	    threshold=free+200;
	    TheFs.NotifyDiskSpace(threshold,KDefaultDrive,stat1);
	    test(stat1==KRequestPending);
	    r=thread.Create(_L("thread6"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	    test_KErrNone(r);
	    thread.Logon( deathStat );
	    thread.Resume();
	    User::WaitForRequest(stat1);
	    //User::After(1000000);
	    test(stat1==KErrNone);
	    User::WaitForRequest( deathStat );
	    test(deathStat==KErrNone);
	    thread.Close();
	    CleanupForThread(task);
	    free=FreeDiskSpace(KDefaultDrive);
	    test(free>threshold);
    }

	// check that CSessionFS::iTheDrive is set in subsession calls
	test.Next(_L("Check iTheDrive and subsessions"));
	if( LffsDrive )
		{
		test.Printf( _L("Skipped.... test not done on LFFS drive\n") );
		}
	else
		{
		RFile f2;
#if defined(__WINS__)
		_LIT(someFile,"X:\\abcdef");
#else
		TBuf<10> someFile=_L("?:\\abcdef");
		TChar c;
		TInt r=RFs::DriveToChar(RemovableDrive,c);
		test_KErrNone(r);
		someFile[0]=(TText)c;
#endif
		_LIT(someDir,"C:\\1234\\");

		r=f2.Create(TheFs,someFile,EFileShareAny|EFileWrite);
		test_KErrNone(r);
		r=TheFs.MkDir(someDir);
		test_KErrNone(r);
		TRequestStatus stat2;
		TInt64 freeC=FreeDiskSpace(EDriveC);
		TInt64 freeD=FreeDiskSpace(RemovableDrive);
		TheFs.NotifyDiskSpace(freeC-4097,EDriveC,stat1);
		TheFs.NotifyDiskSpace(freeD-4097,RemovableDrive,stat2);
		test(stat1==KRequestPending && stat2==KRequestPending);
		// before fix this would result in iTheDrive not being updated in next subsession call
		// therefore this could would not result in a disk space notification
		r=f2.SetSize(8192);
		test_KErrNone(r);
		User::After(1000000);
		User::WaitForRequest(stat2);

		if (stat2!=KErrNone)
			test.Printf(_L("stat2=%d\n"),stat2.Int());
		test(stat2==KErrNone);
		if (stat1!=KRequestPending)
			test.Printf(_L("stat1=%d\n"),stat1.Int());
		test(stat1==KRequestPending);

		f2.Close();
		TheFs.NotifyDiskSpaceCancel();
		test(stat1==KErrCancel);
		r=TheFs.Delete(someFile);
		test_KErrNone(r);
		r=TheFs.RmDir(someDir);
		test_KErrNone(r);
		}

	file.Close();
	r=TheFs.Delete(KFileFiller);
	test_KErrNone(r);
	}



void TestLffsFunctions()
//
// LFFS-specific tests for some functions which may cause a disk
// space change
//
//
	{
	test.Next(_L("test LFFS disk space functions"));
	// create the filler file
	RFile file;
	TInt r=file.Create(TheFs,KFileFiller,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	test_KErrNone(r);
	TInt64 newSpace = FreeDiskSpace(KDefaultDrive)-8192;
	FillDisk(file,newSpace,KDefaultDrive);


	// check file create
	// Creating a file will always allocate space for the inode
	test.Next(_L("check RFile:Create"));
	TThreadTask task=ETaskFileCreateLffs;
	InitialiseForThread(task);
	TInt64 free=FreeDiskSpace(KDefaultDrive);
	TInt64 threshold1=free-64;
	TInt64 threshold2=free-KFileSize3;
	TRequestStatus stat1, stat2;
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
	TheFs.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
	test(stat1==KRequestPending && stat2==KRequestPending);
	RThread thread;
	r=thread.Create(_L("thread7"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	thread.Resume();
	User::WaitForRequest(stat1);
	test(stat1==KErrNone);
	free=FreeDiskSpace(KDefaultDrive);
	test(free<threshold1);
	test(stat2==KRequestPending);
	test(free>threshold2);
	TheFs.NotifyDiskSpaceCancel(stat2);
	User::WaitForRequest(stat2);
	test(stat2==KErrCancel);
	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);

	TInt64 threshold3;
	TRequestStatus stat3;
	RFile file2;
	// don't test for wins urel since cant use RFs::ControlIo
#if defined(_DEBUG)
	// check background thread notification
	test.Next(_L("check Background thread notification"));
	task=ETaskSpin; // create thread to block background thread
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	threshold1=free-64;		// this should occur when we create test file
	threshold2=free+9750;		// some impossible value
	threshold3=free+10000;	// some other impossible value that will never happen
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
	TheFs.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
	TheFs.NotifyDiskSpace(threshold3,KDefaultDrive,stat3);
	test(stat1==KRequestPending && stat2==KRequestPending && stat3==KRequestPending);
	r=thread.Create(_L("thread8"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	thread.Logon( deathStat );
	thread.SetPriority( EPriorityLess );
	thread.Resume();	// start spinning, blocks background thread
	// request background thread to notify a daft value
    TPckgBuf<TInt64> cBuf;
	cBuf() = threshold2;
 	// Hard code the value of ECioBackgroundNotifyDiskSize.
 	// Although the value is enumerated in f32\slffs\lffs_controlio.h this header file is being
 	// removed from the release codeline but retained in the base team development codeline.
	#define ECioBackgroundNotifyDiskSize 10016
	r = TheFs.ControlIo(GetDriveLFFS(), ECioBackgroundNotifyDiskSize, cBuf);
	test( KErrNone==r );
	// create a  file to force some roll-forward
	r=file2.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(stat1);
	test(stat1==KErrNone);
	test(stat2==KRequestPending);
	test(stat3==KRequestPending);
	// kill the spinner thread to allow the background thread to execute
	thread.Kill(KErrNone);
	User::WaitForRequest( deathStat );
	thread.Close();
	// wait for the notifier
	User::WaitForRequest(stat2);
	test( stat2==KErrNone );
	test( stat3==KRequestPending);
	TheFs.NotifyDiskSpaceCancel(stat3);
	User::WaitForRequest(stat3);
	test(stat3==KErrCancel);
	CleanupForThread(task);
	file2.Close();
	TheFs.Delete( KTestFile1 );
#endif

	// check background thread notification again, this time we won't request
	// a special value - check that it notifies normally
	test.Next(_L("check Background thread notification again"));
	task=ETaskSpin; // create thread to block background thread
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	threshold1=free-64;		// this should occur when we create test file
	threshold2=free+9750;		// some impossible value
	threshold3=free+10000;	// some other impossible value that will never happen
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
	TheFs.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
	TheFs.NotifyDiskSpace(threshold3,KDefaultDrive,stat3);
	test(stat1==KRequestPending && stat2==KRequestPending && stat3==KRequestPending);
	r=thread.Create(_L("thread9"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	thread.Logon( deathStat );
	thread.SetPriority( EPriorityLess );
	thread.Resume();	// start spinning, blocks background thread
	// create a  file to force some roll-forward
	r=file2.Create(TheFs,KTestFile1,EFileShareAny|EFileWrite);
	test_KErrNone(r);
	User::WaitForRequest(stat1);
	test(stat1==KErrNone);
	test(stat2==KRequestPending);
	test(stat3==KRequestPending);
	// kill the spinner thread to allow the background thread to execute
	thread.Kill(KErrNone);
	User::WaitForRequest( deathStat );
	thread.Close();
	// wait for the notifier
	test( stat2==KRequestPending );
	test( stat3==KRequestPending);
	TheFs.NotifyDiskSpaceCancel(stat2);
	User::WaitForRequest(stat2);
	test(stat2==KErrCancel);
	TheFs.NotifyDiskSpaceCancel(stat3);
	User::WaitForRequest(stat3);
	test(stat3==KErrCancel);
	CleanupForThread(task);
	file2.Close();
	TheFs.Delete( KTestFile1 );


	file.Close();
	r=TheFs.Delete(KFileFiller);
	test_KErrNone(r);
	}


void TestMultiple()
//
// test muliple requests and multiple sessions
//
	{
	// create the filler file
	RFile file;
	TInt r=file.Create(TheFs,KFileFiller,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	test_KErrNone(r);
	TInt64 free=FreeDiskSpace(KDefaultDrive);
	TInt64 freeSpaceLeft = gMinFileSize << 4;	// 512 * 2^4 = 512 * 16 = 8K
	FillDisk(file,free-freeSpaceLeft,KDefaultDrive);
	TInt size;
	r=file.Size(size);
	test_KErrNone(r);
	test(size>1024);
	test.Printf(_L("filler file size=0x%x\n"),size);

	// test multiple requests
	test.Next(_L("test multiple requests"));
	TThreadTask task = ETaskFileWrite4KB;
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	TInt64 threshold1=free-gMinFileSize;	// 512;
	TInt64 threshold2=free - (gMinFileSize << 2);	// 512 * 4 = 2048;
#if defined(__WINS__)
	TInt64 threshold3=free-70000;	//NTFS over-allocates then reduces when file closed
#else
	TInt64 threshold3=free - (gMinFileSize << 5);	// 512 * 2^5 = 512 * 32 = 16K
#endif
	TRequestStatus stat1,stat2,stat3;
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
	TheFs.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
	TheFs.NotifyDiskSpace(threshold3,KDefaultDrive,stat3);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	RThread thread;
	r=thread.Create(_L("thread1"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	thread.Resume();
	User::After(1000000);
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	test(stat1==KErrNone && stat2==KErrNone);
	test(stat3==KRequestPending);
	free=FreeDiskSpace(KDefaultDrive);
	test(free<threshold1 && free<threshold2 && free>threshold3);
	TheFs.NotifyDiskSpaceCancel(stat3);
	test(stat3==KErrCancel);
	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);

	// test multiple requests again
	test.Next(_L("test multiple requests again"));
	if( LffsDrive )
		{
		// SetSize doesn't use disk space on LFFS
		test.Printf( _L("test skipped on LFFS drive\n") );
		}
	else
		{
		task=ETaskFileSetSize;
		InitialiseForThread(task);	// this also does initialisation for task2
		free=FreeDiskSpace(KDefaultDrive);
		threshold1 = free + (gMinFileSize << 1);	// 512 * 2 = 1024
		threshold2 = free + (gMinFileSize * 12);	// 512 * 12 = 6144
		threshold3 = free - (gMinFileSize << 1);	// 512 * 2 = 1024;
		TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
		TheFs.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
		TheFs.NotifyDiskSpace(threshold3,KDefaultDrive,stat3);
		test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
		r=thread.Create(_L("thread2"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
		test_KErrNone(r);
		thread.Logon( deathStat );
		thread.Resume();
		User::After(10000);
		User::WaitForRequest(stat3);
		test(stat3==KErrNone && stat1==KRequestPending && stat2==KRequestPending);
		free=FreeDiskSpace(KDefaultDrive);
		test(free<threshold1);
		User::WaitForRequest( deathStat );
		thread.Close();
		CleanupForThread(task);
		if(!IsWinsCDrive(KDefaultDrive))
			{
			free=FreeDiskSpace(KDefaultDrive);
			test(stat1==KRequestPending && stat2==KRequestPending);
			file.SetSize(size - (gMinFileSize << 2));	// 512 * 4 = 2048
			free=FreeDiskSpace(KDefaultDrive);
			User::After(1000000);
			User::WaitForRequest(stat1);
			//User::WaitForRequest(stat2);
			test(stat1==KErrNone && stat2==KRequestPending);
			free=FreeDiskSpace(KDefaultDrive);
			test(free>threshold1 && free<threshold2);
			TheFs.NotifyDiskSpaceCancel();
			test(stat2==KErrCancel);
			}
		else
			{
			TheFs.NotifyDiskSpaceCancel();
			test(stat1==KErrCancel&&stat2==KErrCancel&&stat3==KErrNone);
			}
		}

	// test multiple sessions all notified on disk space change
	test.Next(_L("test multiple sessions on same drive"));
	RFs ses2,ses3;
	r=ses2.Connect();
	test_KErrNone(r);
	r=ses3.Connect();
	test_KErrNone(r);
	r=ses2.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	r=ses3.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	task=ETaskFileReplace;
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	test.Printf(_L("free space on default drive = 0x%x\n"),free);
	threshold1=free+gMinFileSize;			// 512
	threshold2=free+(gMinFileSize << 1);	// 1024;
	threshold3=free+(gMinFileSize << 2);	// 2048;
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
	ses2.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
	ses3.NotifyDiskSpace(threshold3,KDefaultDrive,stat3);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	r=thread.Create(_L("thread3"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	thread.Logon( deathStat );
	thread.Resume();
	User::After(1000000);
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	User::WaitForRequest(stat3);
	test(stat1==KErrNone && stat2==KErrNone && stat3==KErrNone);
	free=FreeDiskSpace(KDefaultDrive);
	test(free>threshold1 && free>threshold2 && free>threshold3);
	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);

	// test NotifyDiskSpaceCancel()
	test.Next(_L("test RFs:NotifyDiskSpaceCancel"));
	free=FreeDiskSpace(KDefaultDrive);
	TheFs.NotifyDiskSpace(free-100,KDefaultDrive,stat1);
	ses2.NotifyDiskSpace(free-100,KDefaultDrive,stat2);
	ses3.NotifyDiskSpace(free-100,KDefaultDrive,stat3);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	TheFs.NotifyDiskSpaceCancel();
	test(stat1==KErrCancel&&stat2==KRequestPending&&stat3==KRequestPending);
	ses2.NotifyDiskSpaceCancel(stat2);
	test(stat2==KErrCancel&&stat3==KRequestPending);
	ses3.NotifyDiskSpaceCancel();
	test(stat3==KErrCancel);

	if( !LffsDrive )
		{
		TInt sessionDrive;
		r=RFs::CharToDrive(gSessionPath[0],sessionDrive);
		test_KErrNone(r);
		if(sessionDrive!=RemovableDrive)
			{
			// first create a file on the removable drive
			RFile file2;
			TFileName file2name=RemovableDriveBuf;
			file2name+=_L("F32-TST\\testfile1");
			TheFs.Delete(file2name);
			r=file2.Create(TheFs,file2name,EFileShareAny|EFileWrite);
			test_KErrNone(r);
			r=file2.SetSize(KFileSize3);
			test_KErrNone(r);
			// test multiple sessions not notified on disk space change on wrong drive
			test.Next(_L("test multiple sessions on different drives"));
			task=ETaskFileReplace;
			InitialiseForThread(task);
			TInt64 freeDef=FreeDiskSpace(KDefaultDrive);
			TInt64 freeRem=FreeDiskSpace(RemovableDrive);
			threshold1=freeDef + (gMinFileSize << 1);	// 1024;
			threshold2=freeRem + (gMinFileSize << 1);	// 1024;
			TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
			ses2.NotifyDiskSpace(threshold2,RemovableDrive,stat2);
			test(stat1==KRequestPending&&stat2==KRequestPending);
			r=thread.Create(_L("thread4"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
			test_KErrNone(r);
			thread.Logon( deathStat );
			thread.Resume();
			User::After(1000000);
			User::WaitForRequest(stat1);
			test(stat1==KErrNone && stat2==KRequestPending);
			free=FreeDiskSpace(KDefaultDrive);
			test(free>threshold1);
			User::WaitForRequest( deathStat );
			thread.Close();
			CleanupForThread(task);
			TheFs.NotifyDiskSpaceCancel(stat2);
			test(stat2==KRequestPending);
			ses2.NotifyDiskSpaceCancel(stat2);
			test(stat2==KErrCancel);
			file2.Close();
			r=TheFs.Delete(file2name);
			test_KErrNone(r);
			}
		}

	ses2.Close();
	ses3.Close();


	file.Close();
	r=TheFs.Delete(KFileFiller);
	test_KErrNone(r);

	}


void TestLffsMultiple()
//
// test muliple requests and multiple sessions speicifcally for LFFS drive
//
	{
	// create the filler file
	RFile file;
	TInt r=file.Create(TheFs,KFileFiller,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	test_KErrNone(r);
	TInt64 free=FreeDiskSpace(KDefaultDrive);
	FillDisk(file,free-8192,KDefaultDrive);
	TInt size;
	r=file.Size(size);
	test_KErrNone(r);
	test.Printf(_L("filler file size=0x%x\n"),size);


	// test multiple requests again
	test.Next(_L("test multiple requests on LFFS") );

	TThreadTask task=ETaskFileCreateLffs;
	InitialiseForThread(task);	// this also does initialisation for task2
	free=FreeDiskSpace(KDefaultDrive);
	TInt64 threshold1=free+8192;
	TInt64 threshold2=free+700; //increased threshold as LFFS, unpredicatably, can release drive space
	TInt64 threshold3=free-64;
	TRequestStatus stat1, stat2, stat3;
//test.Printf(_L("set up notifiers"));
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
	TheFs.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
	TheFs.NotifyDiskSpace(threshold3,KDefaultDrive,stat3);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	RThread thread;
	r=thread.Create(_L("thread10"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
//	test.Printf(_L("Resuming other thread"));
	thread.Resume();
	User::After(10000);
	User::WaitForRequest(stat3);
	test(stat3==KErrNone && stat1==KRequestPending && stat2==KRequestPending);
	free=FreeDiskSpace(KDefaultDrive);
	test(free<threshold1);
	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
//	test.Printf(_L("free =%d and %d"),free.Low(), free.High());
//	test.Printf(_L("stat1=%d, stat2=%d"),stat1,stat2);
	test(stat1==KRequestPending && stat2==KRequestPending);
	file.SetSize(6144);
	User::After(1000000);
	User::WaitForRequest(stat2);
	test(stat1==KRequestPending && stat2==KErrNone);
	free=FreeDiskSpace(KDefaultDrive);
	test(free<threshold1 && free>threshold2);
	TheFs.NotifyDiskSpaceCancel();
	User::WaitForRequest( stat1 );
	test(stat1==KErrCancel);



	TInt sessionDrive;
	r=RFs::CharToDrive(gSessionPath[0],sessionDrive);
	test_KErrNone(r);
	if(sessionDrive!=EDriveC)
		{
		// test multiple sessions not notified on disk space change on wrong drive
		test.Next(_L("test multiple sessions on different drives"));

		RFs ses2,ses3;
		r=ses2.Connect();
		test_KErrNone(r);
		r=ses3.Connect();
		test_KErrNone(r);
		r=ses2.SetSessionPath(gSessionPath);
		test_KErrNone(r);
		r=ses3.SetSessionPath(gSessionPath);
		test_KErrNone(r);

		// first create a file on the C:\ drive
		RFile file2;
		TFileName file2name=_L("C:\\");
		file2name+=_L("F32-TST\\");
		r=TheFs.MkDir(file2name);
		test( KErrNone==r || KErrAlreadyExists==r );
		file2name+=_L("testfile1");
		TheFs.Delete(file2name);
		r=file2.Create(TheFs,file2name,EFileShareAny|EFileWrite);
		test_KErrNone(r);
		WriteToFile( file2, KFileSize3 );

		task=ETaskFileReplace;
		InitialiseForThread(task);
		TInt64 freeLffs=FreeDiskSpace(KDefaultDrive);
		TInt64 freeD=FreeDiskSpace(EDriveC);
		threshold1=freeLffs+1024;
		threshold2=freeD+1024;
		TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
		ses2.NotifyDiskSpace(threshold2,EDriveC,stat2);
		test(stat1==KRequestPending&&stat2==KRequestPending);
		r=thread.Create(_L("thread11"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
		test_KErrNone(r);
		thread.Logon( deathStat );
		thread.Resume();
		User::After(1000000);
		User::WaitForRequest(stat1);
		test(stat1==KErrNone && stat2==KRequestPending);
		free=FreeDiskSpace(KDefaultDrive);
		test(free>threshold1);
		User::WaitForRequest( deathStat );
		thread.Close();
		CleanupForThread(task);
		TheFs.NotifyDiskSpaceCancel(stat2);
		test(stat2==KRequestPending);
		ses2.NotifyDiskSpaceCancel(stat2);
		User::WaitForRequest( stat2 );
		test(stat2==KErrCancel);
		file2.Close();
		r=TheFs.Delete(file2name);
		test_KErrNone(r);
		ses2.Close();
		ses3.Close();
		}



	file.Close();
	r=TheFs.Delete(KFileFiller);
	test_KErrNone(r);

	}


void TestChangeNotification()
//
// test that disk space notification works with (extended) change notification
//
	{
	// create a filler file
	RFile file;
	TInt r=file.Create(TheFs,KFileFiller,EFileShareAny|EFileWrite|EFileWriteDirectIO);
	test_KErrNone(r);
	TInt64 free=FreeDiskSpace(KDefaultDrive);
	// use 8KB in filler file
	FillDisk(file,free-8192,KDefaultDrive);

	// test change notification when no disk space change
	test.Next(_L("test change notification"));
	TThreadTask task = ETaskNoChange1;
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	TInt64 threshold1=free+gMinFileSize;
	TInt64 threshold2=free-gMinFileSize;
	TRequestStatus stat1,stat2,stat3;
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);		//more free space becoming available
	TheFs.NotifyDiskSpace(threshold2,KDefaultDrive,stat2);
	TheFs.NotifyChange(ENotifyAll,stat3);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	RThread thread;
	r=thread.Create(_L("thread1"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	TRequestStatus deathStat;
	thread.Logon( deathStat );
	thread.Resume();
	User::After(1000000);
	User::WaitForRequest(stat3);
	if(!LffsDrive)
		test(stat1==KRequestPending && stat2==KRequestPending && stat3==KErrNone);
	else
		test(stat2==KRequestPending && stat3==KErrNone);	//As below

	TheFs.NotifyDiskSpaceCancel();
	if(!LffsDrive)
		test(stat1==KErrCancel && stat2==KErrCancel);
	else
		test(stat2==KErrCancel);	//is invalid for LFFS as can free up space un expectedly

	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);

	// Have three different sessions
	// do an operation that will cause the change notification
	// and disk change notification to be signalled
	test.Next(_L(" test change notification and disk space notification"));
	RFs session2,session3;
	r=session2.Connect();
	test_KErrNone(r);
	r=session3.Connect();
	test_KErrNone(r);
	r=session2.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	r=session3.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	task=ETaskFileWrite;
	InitialiseForThread(task);
	free=FreeDiskSpace(KDefaultDrive);
	threshold1=free-400;
	TheFs.NotifyDiskSpace(threshold1,KDefaultDrive,stat1);
	session2.NotifyChange(ENotifyAll,stat2);
	session3.NotifyChange(ENotifyAll,stat3,KTestFile1);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	r=thread.Create(_L("thread2"),ThreadFunction,KStackSize,KHeapSize,KHeapSize,(TAny*)task);
	test_KErrNone(r);
	thread.Logon( deathStat );
	thread.Resume();
	User::After(1000000);
	User::WaitForRequest(stat1);
	User::WaitForRequest(stat2);
	User::WaitForRequest(stat3);
	test(stat1==KErrNone && stat2==KErrNone && stat3==KErrNone);
	free=FreeDiskSpace(KDefaultDrive);
	test(free<threshold1);
	User::WaitForRequest( deathStat );
	thread.Close();
	CleanupForThread(task);

	// check cancellation of change notification and disk space notification
	// on same session
	test.Next(_L("test cancellation of notifications"));
	TheFs.NotifyDiskSpace(FreeDiskSpace(KDefaultDrive)-512,KDefaultDrive,stat1);
	TheFs.NotifyChange(ENotifyAll,stat2);
	TheFs.NotifyChange(ENotifyAll,stat3,KTestFile1);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	TheFs.NotifyChangeCancel();
	test(stat1==KRequestPending&&stat2==KErrCancel&&stat3==KErrCancel);
	TheFs.NotifyDiskSpaceCancel();
	test(stat1==KErrCancel);
	// request change notification again
	TheFs.NotifyDiskSpace(FreeDiskSpace(KDefaultDrive)-512,KDefaultDrive,stat1);
	TheFs.NotifyChange(ENotifyAll,stat2);
	TheFs.NotifyChange(ENotifyAll,stat3,KTestFile1);
	test(stat1==KRequestPending&&stat2==KRequestPending&&stat3==KRequestPending);
	TheFs.NotifyDiskSpaceCancel();
	test(stat1==KErrCancel&&stat2==KRequestPending&&stat3==KRequestPending);
	TheFs.NotifyChangeCancel();
	test(stat1==KErrCancel&&stat2==KErrCancel&&stat3==KErrCancel);


	session2.Close();
	session3.Close();

	file.Close();
	r=TheFs.Delete(KFileFiller);
	test_KErrNone(r);

	}

//-------------------------------------------------------------------------------------------------
// Test the fix for:
// ou1cimx#410349 Not getting any Notification from RFs::NotifyDiskSpace() for E and F drive when when tested multiple
// 
// Action: Enable a plugin to intercept RFs::Delete, and test RFs::Delet can still trigger disk space
// notification
//-------------------------------------------------------------------------------------------------

_LIT(KPreModifierPluginFileName,"premodifier_plugin");
_LIT(KPreModifierPluginName,"PreModifierPlugin");
const TUint KTestFileSize = KKilo * 100;

#define SAFETEST_KErrNone(a)        if(a != KErrNone)\
                                        {\
                                        TheFs.DismountPlugin(KPreModifierPluginName);\
                                        TheFs.RemovePlugin(KPreModifierPluginName);\
                                        test_KErrNone(a);\
                                        }

TInt PluginTestThreadFunction(TAny*)
    {
    RTest test(_L("PluginTestThreadFunction"));
    RFs fs;
    fs.Connect();
    
    TInt r = fs.SetSessionPath(gSessionPath);
    test_KErrNone(r);
    
    RFile file;
    r = file.Create(fs, KTestFile1, EFileShareAny|EFileWrite);
    test_KErrNone(r);
    r = file.SetSize(KTestFileSize);
    test_KErrNone(r);
    file.Close();
      
    User::After(5000000); // wait for 5 seconds, to ensure first notification received.
    
    r = fs.Delete(KTestFile1);
    test_KErrNone(r);
    
    fs.Close();
    return KErrNone;
    }

void TestDiskSpaceNotifyWithPlugin()
    {
    test.Next(_L("Test Disk Space Notify With Plugin"));
      
    TInt drive;
    TInt r = RFs::CharToDrive(gSessionPath[0],drive);
    SAFETEST_KErrNone(r);
    Format(drive);
    
    r = TheFs.MkDirAll(gSessionPath);
    SAFETEST_KErrNone(r);
     
    r = TheFs.AddPlugin(KPreModifierPluginFileName);
    SAFETEST_KErrNone(r);

    r = TheFs.MountPlugin(KPreModifierPluginName);
    SAFETEST_KErrNone(r);
    
    TInt64 free = FreeDiskSpace(drive);
    TInt64 threshold = free - KTestFileSize + 1;
    
    TRequestStatus status;
    TRequestStatus statusDeath;
    
    TheFs.NotifyDiskSpace(threshold, drive, status);
    
    RThread thread;
    r = thread.Create(_L("PluginTestThread"), PluginTestThreadFunction, KStackSize, KHeapSize, KHeapSize, NULL);
    SAFETEST_KErrNone(r);
    thread.Logon(statusDeath);
    thread.Resume();
    
    User::WaitForRequest(status);
    SAFETEST_KErrNone(status.Int());
    
    TheFs.NotifyDiskSpace(threshold, drive, status);
    User::WaitForRequest(status);
    SAFETEST_KErrNone(status.Int());
    
    User::WaitForRequest(statusDeath);
    SAFETEST_KErrNone(statusDeath.Int());
    thread.Close();
    
    r = TheFs.DismountPlugin(KPreModifierPluginName);
    SAFETEST_KErrNone(r);

    r = TheFs.RemovePlugin(KPreModifierPluginName);
    SAFETEST_KErrNone(r);

    Format(drive);
    }

GLDEF_C void CallTestsL()
//
// Do all tests
//
	{
	TInt r = KErrNone;
	TInt driveNumber;
	if(IsTestingLFFS())
		{
		LffsDrive = ETrue;
		r = TheFs.CharToDrive( gSessionPath[0], driveNumber );
		test( KErrNone == r );
		}

	// at present internal ram drive not tested - the test should allow for fact
	// that memory allocation will affect the free disk space on the internal ram drive

	// pc file system c drive is also not tested - the test should allow for the fact
	// that other windows processes may affect the free disk space
#ifdef __WINS__
	if(gSessionPath[0]==(TText)'C')
#else
	// check if gSessionPath drive is RAM drive
	TDriveInfo driveInfo;
	test(KErrNone == TheFs.CharToDrive(gSessionPath[0], driveNumber));
	test(KErrNone == TheFs.Drive(driveInfo, driveNumber));
	if(driveInfo.iType == EMediaRam)
#endif
		{
#ifdef __WINS__
		test.Printf( _L("C:\\ is not tested, test will exit") );
#else
		test.Printf( _L("%c:\\ is not tested (is RAM drive), test will exit"), gSessionPath[0]);
#endif
		return;
		}
	//Test uses C drive as secondary drive so test can't be tested on that drive
	r = TheFs.CharToDrive(gSessionPath[0], driveNumber);
	test_KErrNone(r);
	if(driveNumber == EDriveC)
		{
		test.Printf(_L("Test uses C drive as secondary drive so test can't be test on C drive, test will exit"));
		return;
		}

	Initialise();
	TestErrorConditions();
	TestCancellation();
	TestFunctions();
	TestMultiple();
	if( !LffsDrive )
		{
		TestDiskNotify();
		}

	TestChangeNotification();
	TestDiskSpaceNotifyWithPlugin();

	if( LffsDrive )
		{

		TestLffsFunctions();
		TestLffsMultiple();
		}
	}

