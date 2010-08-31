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
// f32test\server\b_open.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_chlffs.h"
#include "f32_test_utils.h"

using namespace F32_Test_Utils;

#ifdef __WINS__
#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#include <windows.h>
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union
#endif

GLDEF_D RTest test(_L("B_OPEN"));

LOCAL_D const TInt KMaxFiles=200;
LOCAL_D TBuf<32> nameBuf[KMaxFiles];
LOCAL_D TBuf<0x100> nameBuf1;
LOCAL_D RFile chan[KMaxFiles];
LOCAL_D RFile chan1;
LOCAL_D TFileName fBuf;
LOCAL_D TVolumeInfo vInfo;
LOCAL_D TInt LeaveMemFree;

LOCAL_C void testOpenFiles()
//
// Open files till memory is full
// Write, seek, read, seteof, close, check and delete all files.
//
    {
	
	TChar currentDrive=gSessionPath[0];
	TInt driveNum;
	TInt r=TheFs.CharToDrive(currentDrive,driveNum);
	test_KErrNone(r);

    TInt i=0;
	TInt totalRam;
    FOREVER
        {

#if defined(__WINS__)
		DWORD sectorsPerCluster;
		DWORD bytesPerSector;
		DWORD freeClusters;
		DWORD sizeClusters;
		BOOL b=GetDiskFreeSpaceA("C:\\",&sectorsPerCluster,&bytesPerSector,&freeClusters,&sizeClusters);
		test(b==TRUE);
		totalRam=sizeClusters*sectorsPerCluster*bytesPerSector;
#else
		TMemoryInfoV1Buf memInfoBuf;
		UserHal::MemoryInfo(memInfoBuf);
		totalRam=memInfoBuf().iTotalRamInBytes;
#endif
        test.Printf(_L("Open %u\n"),i);
        nameBuf[i].Format(_L("B_OPEN test file %d"),i);
		r=chan[i].Replace(TheFs,nameBuf[i],EFileStream|EFileWrite);
		if (r==KErrNone)
			{
			r=chan[i].Write(_L8("SomeText"));
			if ((gDriveCacheFlags & EFileCacheWriteOn) && (r == KErrNone))
				r = chan[i].Flush();
			if (r==KErrDiskFull)
				{
				chan[i].Close();
				test(TheFs.Delete(nameBuf[i])==KErrNone);
				}
			}
		if (r==KErrDiskFull)
			break;
		if (r==KErrNoMemory)
			break;
		if (r!=KErrNone)
			{
			test.Printf(_L("ERROR: RFile::Replace returned %d\n"),r);
			test(EFalse);
			}

        r=TheFs.Volume(vInfo);
		test_KErrNone(r);
		if (driveNum==EDriveC)
	        test.Printf(_L("VInfo size=0x%x free=0x%x TotalRam = 0x%x\n"),vInfo.iSize,vInfo.iFree,totalRam);
        test(vInfo.iFree<=vInfo.iSize);

//      r=TheFs.Volume(vInfo,(driveNum==EDriveC) ? EDriveD : EDriveC);
//		if (r==KErrNone);
//		test(vInfo.iFree<=vInfo.iSize);
	    i++;
        if (i==KMaxFiles)
            {
            if (IsTestingLFFS())
                break;
            else
                test.Panic(_L("Too many files opened"));
            }
        }

	test.Printf(_L("Created %d extra files\n"),i);
	const TInt n_files=i;
	r=TheFs.Volume(vInfo);
	test_KErrNone(r);
	if (driveNum==EDriveC && (vInfo.iDrive.iMediaAtt & KMediaAttVariableSize))
		{
		test(vInfo.iSize<=totalRam);
		test(vInfo.iFree<=totalRam);
		}
    test(vInfo.iFree<=vInfo.iSize);

	test.Next(_L("SetSize to each file"));
    for (i=0;i<n_files;i++)
        {
		r=chan[i].SetSize(0);
		test_KErrNone(r);
        }

	test.Next(_L("Write to each file"));
    TBuf8<3> numBuf;
    for (i=0;i<n_files;i++)
        {
        test.Printf(_L("Write %u\n"),i);
		numBuf.Format(_L8("%d"),i);
		r=chan[i].Write(0,numBuf);
		test_KErrNone(r);
        }

	test.Next(_L("Seek on each file"));
    for (i=0;i<n_files;i++)
        {
        test.Printf(_L("Seeking %u\n"),i);
        TInt pos=0;
        r=chan[i].Seek(ESeekStart,pos);
		test_KErrNone(r);
        }

	test.Next(_L("Read from each file"));
    TBuf8<3> checkBuf;
	for (i=0;i<n_files;i++)
        {
        test.Printf(_L("Read %u\n"),i);
        r=chan[i].Read(checkBuf,3);
		test_KErrNone(r);
		numBuf.Format(_L8("%d"),i);
		test(numBuf==checkBuf);
        }

	test.Next(_L("Set size of each file"));
    for (i=0;i<n_files;i++)
        {
        test.Printf(_L("Set size %u\n"),i);
        r=chan[i].SetSize(i);
		test_KErrNone(r);
        }

    r=TheFs.Volume(vInfo);
	test_KErrNone(r);
	if (driveNum==EDriveC && (vInfo.iDrive.iMediaAtt & KMediaAttVariableSize))
		{
	    test(vInfo.iSize<=totalRam);
		test(vInfo.iFree<=totalRam);
		}
    test(vInfo.iFree<=vInfo.iSize);

	test.Next(_L("Close each file"));
    for (i=0;i<n_files;i++)
        {
        test.Printf(_L("Close %u\n"),i);
        chan[i].Close();
        }

	test.Next(_L("Open each file"));

	TInt n_files_open;
    for (n_files_open=0; n_files_open < n_files; n_files_open++)
        {
        r = chan[n_files_open].Open(TheFs,nameBuf[n_files_open],EFileRead|EFileStream);
		test.Printf(_L("Open(%d) ret %d\n"), n_files_open, r);
		if (r != KErrNone)
			{
			if (driveNum==EDriveC && (vInfo.iDrive.iMediaAtt & KMediaAttVariableSize))
				break;
			else
				test(0);
			}
        }

	test.Next(_L("Check size of each file"));
    for (i=0;i<n_files_open;i++)
        {
        test.Printf(_L("Check %u\n"),i);
        TInt size;
		TInt r=chan[i].Size(size);
		test.Printf(_L("size of file %u is %d ret %d\n"),i,size, r);
		test_KErrNone(r);
		test_Equal(i,size);
        }

	test.Next(_L("Close each file"));
    for (i=0;i<n_files_open;i++)
        {
        test.Printf(_L("Close %u\n"),i);
        chan[i].Close();
        }
	
	test.Next(_L("Delete files"));
    for (i=0;i<n_files;i++)
        {
        test.Printf(_L("Delete %u\n"),i);
        r=TheFs.Delete(nameBuf[i]);
		test_KErrNone(r);
        }
    }



LOCAL_C void InitTest()
//
// Create a large file
//
	{
	TInt fileNumber = 0;
	TBool fileSizeTruncated;

	do
		{
		fileSizeTruncated = EFalse;
		nameBuf1.Format(_L("\\Hooge Test File for B_OPEN %d"), fileNumber++);

		TInt r=chan1.Replace(TheFs,nameBuf1,EFileStream|EFileWrite);
		test_KErrNone(r);
		r=TheFs.Volume(vInfo);
		test_KErrNone(r);
		TInt64 size;
		LeaveMemFree = 0x400; // ???
		if (vInfo.iFree>LeaveMemFree)
			size=vInfo.iFree-LeaveMemFree;
		else
			size=0;

		const TInt KMaxFileSize = 0x40000000;

		// test as 64 bit numbers in case size is very large (eg. enough that TInt is
		// not large enough to hold it)
		TInt64 KMaxFileSize64 = MAKE_TINT64(0, KMaxFileSize);

		test.Printf(_L("Free space available = %08x:%08x\n"), I64HIGH(size), I64LOW(size));
		if (size > KMaxFileSize64)
			{
			size = KMaxFileSize;
			test.Printf(_L("Truncated to %d to avoid current FAT FSY file size limit !!!\n"), size);
			fileSizeTruncated = ETrue;
			}

		TFileName sessionPath;
		TheFs.SessionPath(sessionPath);
		TBuf<32> message=_L("?: has %ld bytes free\n");
		message[0]=sessionPath[0];
		test.Printf(message,vInfo.iFree);
		if (((vInfo.iDrive.iMediaAtt)&KMediaAttVariableSize)==0)
			{
			// Not a variable sized drive, so should be safe to to just create big file
			test.Printf(_L("Creating %S, 0x%08lx\n"),&nameBuf1,size);
			r=chan1.SetSize((TUint)size);
			}
		else
			{
			// Variable sized drive (proabably RAM drive) needs a bit of special treatment
			// Use a binary search to allocate largest sized file possible...
			test.Printf(_L("Creating %S, 0x%08x\n"),&nameBuf1,size);
			TInt lo = 0;
			TInt hi = (TInt)size;
			const TInt KSizeGranularity = 0x200; // must be power-of-2
			while(hi-lo>KSizeGranularity && r==KErrNone)
				{
				TInt trySize = (lo+hi)/2;
				trySize &= ~(KSizeGranularity-1);
				r = chan1.SetSize((TUint)trySize);
				if(r==KErrNone)
					{
					size = trySize;
					lo = trySize;
					}
				else if(r==KErrDiskFull)
					{
					hi = trySize;
					r = KErrNone;
					}
				}
			if(r==KErrNone)
				{
				// reduce size to leave some free for tests...
				LeaveMemFree = 4096*4; // best leave several RAM pages worth of space so rest of test has some memory
				size -= LeaveMemFree;
				r = chan1.SetSize((TUint)size);
				}
			}

		if (r!=KErrNone)
			{
			test.Printf(_L("ERROR: Creating large file failed %d\n"),r);
			test(EFalse);
			}
		test.Printf(_L("Created %S, 0x%08x\n"),&nameBuf1,size);
		chan1.Close();
		}
	while (fileSizeTruncated);

	}

LOCAL_C void Cleanup()
//
// Cleanup test files
//
	{

	TInt r=TheFs.Delete(nameBuf1);
	test_KErrNone(r);
	r=TheFs.RmDir(gSessionPath);
	test_KErrNone(r);
	}


GLDEF_C void CallTestsL()
//
// Call tests that may leave
//
	{
    if (Is_SimulatedSystemDrive(TheFs, CurrentDrive()))
        {
		// These tests try to create a huge file to fill up the drive.
		// This fails on WINS with drives with > 1/2G free because
		// RFile::SetSize() (among other things) only takes a TInt.
		test.Printf(_L("Skipping B_OPEN on PlatSim/Emulator drive %C:\n"), gSessionPath[0]);
		return;
        }

	CreateTestDirectory(_L("\\B_OPEN\\"));
	InitTest();
	testOpenFiles();
	Cleanup();
	}

