// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\t_ext1.cpp
// 
//

#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"
#include "f32_test_utils.h"

using namespace F32_Test_Utils;


RTest test(_L("T_PROXYDRIVE1"));


TInt GetRemovableDrive(TInt aDriveNumber)
	{
	RLocalDrive d;
	TBool flag=EFalse;

	//Find the local drive number corresponding to aDriveNumber
	TMediaSerialNumber serialNum;
	TInt r = TheFs.GetMediaSerialNumber(serialNum, aDriveNumber);
	if(r!= KErrNone)
		return r;

	TInt len = serialNum.Length();
	test.Printf(_L("Serial number (len %d) :\n"), len);

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

		if ((caps.iDriveAtt & KDriveAttRemovable) == 0)
			{
			continue;
			}

		TPtrC8 localSerialNum(caps.iSerialNum, caps.iSerialNumLength);
		if (serialNum.Compare(localSerialNum) == 0)
			{
			d.Close();
			test.Printf(_L("found removable drive: local drive number %d\n"), n);
			return n;
			}

		d.Close();
		}

	return KErrNotFound;
	}




void CallTestsL()
	{
    TInt drive;
	TInt err=RFs::CharToDrive(gDriveToTest,drive);
	test.Start(_L("Starting Test - T_PROXYDRIVE1"));
	test(err==KErrNone);

    PrintDrvInfo(TheFs, drive);


	_LIT(KBitProxyDriveName, "t_bitproxydrive.pxy");
	_LIT(KBitProxyDrive, "bitproxydrive");

	TInt r;

	TInt localDriveNumber = GetRemovableDrive(drive); //-- local _physical_ drive number
	if (localDriveNumber < 0)
		{
		test.Printf(_L("Not a removable drive, skipping test\n"));
		return;
		}


	test.Next(_L("Adding and then removing a proxy drive with an open subsession"));
	r = TheFs.AddProxyDrive(KBitProxyDriveName);
	test.Printf(_L("AddProxyDrive(%S) r %d\n"), &KBitProxyDriveName, r);
	test(r == KErrNone || r == KErrAlreadyExists);

	TPckgBuf<TInt> p1; p1() = localDriveNumber;
	TBuf<1> p2;
	TInt driveNumber = EDriveM;

    //-- this is a hack - mount the proxy drive to the existing one with alive file system just to check
    //-- that it works.

	r = TheFs.MountProxyDrive(driveNumber, KBitProxyDrive, &p1, &p2);
	test.Printf(_L("MountProxyDrive(%d, %S) r %d\n"), driveNumber, &KBitProxyDrive, r);
	test (r >= 0);

    //-- query existing file system name on the drive that we are be parasiting on.
    TFSName fsName;
    r = TheFs.FileSystemName(fsName, drive);
    test(r == KErrNone);


	r = TheFs.MountFileSystem(fsName, driveNumber);
	test.Printf(_L("MountFileSystem(%S) r %d\n"), &fsName, r);
	test(r == KErrNone);



	RFs fs;
	r = fs.Connect();
	test(r == KErrNone);


	TPath dirPath = _L("?:\\*");
	dirPath[0] = (TUint8) ('A' + driveNumber);
	RDir dir;
	r = dir.Open(fs, dirPath, KEntryAttNormal);
	test.Printf(_L("RDir::Open(%S) r %d\n"), &dirPath, r);


	r = TheFs.DismountFileSystem(fsName, driveNumber);
	test.Printf(_L("DismountFileSystem(%S) r %d\n"), &fsName, r);
	test (r == KErrInUse);

	// dismount failed - attempt a forced dismount
	TRequestStatus stat;
	TheFs.NotifyDismount(driveNumber, stat, EFsDismountForceDismount);
	User::WaitForRequest(stat);
	r = stat.Int();
	test.Printf(_L("DismountFileSystem(%S, EFsDismountForceDismount) r %d\n"), &fsName, r);
	test (r == KErrNone);

	r = TheFs.DismountProxyDrive(driveNumber);
	test.Printf(_L("DismountProxyDrive(%d) r %d\n"), driveNumber, r);
	test (r == KErrNone);

	test.Printf(_L("Calling RemoveProxyDrive()...\n"));
	r = TheFs.RemoveProxyDrive(KBitProxyDrive);
	test.Printf(_L("RemoveProxyDrive() r %d\n"), r);
	test (r == KErrNone);
	User::After(1000000);

	test.Printf(_L("closing dir (%S)....\n"), &dirPath, r);
	dir.Close();

	test.Printf(_L("closing file session()....\n"));
	fs.Close();


	test.End();
	test.Close();
	}
