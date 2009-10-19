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
// f32test\server\t_chlffs.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <hal.h>

// Now non-t_main programs need to chk if they are running on lffs,
#include "t_server.h"
#include "t_chlffs.h"


const TInt KInvalidDriveLetter=-1;

LOCAL_D TInt LFFSdriveNumber=KInvalidDriveLetter;
LOCAL_D TBool LFFSTesting=EFalse;
_LIT(KLFFSName,"Lffs");


LOCAL_C void FormatLFFS(RFs &anFsSession,TDes &aDrive)
//
// Format the LFFS drive
//
	{
    RFormat format;
    TInt count;
    TInt r;
    
    test.Printf(_L("Format LFFS drive %S\r\n"), &aDrive);
    r=format.Open(anFsSession,aDrive,EHighDensity,count);
    test.Printf(_L("Format open done. Count = %d\r\n"), count);
    test(r==KErrNone);
    
    while (count)
		{
        TInt r=format.Next(count);
    	test.Printf(_L("Format next done. Count = %d\r\n"), count);
        test(r==KErrNone);
    	}
    
    format.Close();
	}

GLDEF_C TBool IsFileSystemLFFS(RFs &aFsSession,TInt aDrive)
//
// return true if lffs on aDrive
//
	{
	TFileName f;
	TInt r=aFsSession.FileSystemName(f,aDrive);
	test(r==KErrNone || r==KErrNotFound);
	return (f.CompareF(KLFFSName)==0);
	}

GLDEF_C TInt CheckLFFSDriveForPlatform()
//
// Check the LFFS drive number for the current platform
//
	{
	TInt lffsDrvNum;

	TInt uid;
    TInt r=HAL::Get(HAL::EMachineUid,uid);
    test(r==KErrNone);
    
    if (uid==HAL::EMachineUid_Brutus)
		{
        lffsDrvNum=EDriveK;
    	test.Printf(_L("Test is running on BRUTUS\r\n"));
		}
    else if (uid==HAL::EMachineUid_Win32Emulator)
		{
        lffsDrvNum=EDriveW;
    	test.Printf(_L("Test is running on WINS Pc\r\n"));
		}
	else if (uid == HAL::EMachineUid_Integrator)
		{
		lffsDrvNum=EDriveK;
		test.Printf(_L("Test is running on INTEGRATOR\r\n"));
		}
	else if (uid == HAL::EMachineUid_Assabet)
		{
		lffsDrvNum=EDriveK;
		test.Printf(_L("Test is running on Assabet\r\n"));
		}
    else
		lffsDrvNum=KInvalidDriveLetter;
	return(lffsDrvNum);
	}

GLDEF_C TBool CheckMountLFFS(RFs &anFsSession,TChar aDriveLetter)
//
// Check if test to be performed on LFFS drive. Mount the ELFFS.FSY if
// necessary
//
	{

    test.Next(_L("Check if LFFS drive (Mount LFFS if required)"));

	TInt r,drvNum;
	TBuf<4> lffsDriveLetter=_L("?:\\");

	// first check if the lffs is mounted on the drive
	r=anFsSession.CharToDrive(aDriveLetter,drvNum);
	test(r==KErrNone);
	if (IsFileSystemLFFS(anFsSession,drvNum))
		{
		lffsDriveLetter[0]=(TText)aDriveLetter;
		test.Printf(_L("Testing an LFFS drive (%S)"), &lffsDriveLetter);
		test.Printf(_L("LFFS already mounted on drive %S\r\n"), &lffsDriveLetter);
		LFFSdriveNumber=drvNum;
		LFFSTesting=ETrue;
		return(ETrue);
		}

	// check if platform expects lffs to be mounted on specified drive 
	TInt lffsDrvNum=CheckLFFSDriveForPlatform();
	if (drvNum!=lffsDrvNum)
		{
        test.Printf(_L("Not testing an LFFS drive\n"));
        return(EFalse);
    	}

	lffsDriveLetter[0]=(TText)aDriveLetter;
    test.Printf(_L("Testing an LFFS drive (%S)"), &lffsDriveLetter);
	LFFSdriveNumber=lffsDrvNum;
	LFFSTesting=ETrue;

    test.Next(_L("Load device driver: MEDLFS"));
    r=User::LoadPhysicalDevice(_L("MEDLFS"));
    test(r==KErrNone || r==KErrAlreadyExists);
    
    test.Next(_L("Add file system: ELFFS"));
    r=anFsSession.AddFileSystem(_L("ELFFS"));
    test(r==KErrNone || r==KErrAlreadyExists);
    
    TFullName name;
    r=anFsSession.FileSystemName(name,LFFSdriveNumber);
    test(r==KErrNone || r==KErrNotFound);

    if (name.MatchF(_L("Lffs")) != 0)
		{
        // Some other file system is at the "Lffs" drive. 
        if (name.Length() != 0)
			{
			// Not allowed to dismount the file system from the drive associated
			// with the default path - so temporarily change the default path.
            test.Printf(_L("Dismounting %S on drive %S\r\n"), &name, &lffsDriveLetter);
            r=anFsSession.DismountFileSystem(name,LFFSdriveNumber);
            test(r==KErrNone);

			}
    
        test.Printf(_L("Mount LFFS on drive %S\r\n"),&lffsDriveLetter);
        r=anFsSession.MountFileSystem(_L("Lffs"), LFFSdriveNumber);
        test.Printf(_L("  Mount result %d\r\n"), r);
        test(r==KErrNone || r==KErrCorrupt || r==KErrNotReady);
    
        if (r==KErrCorrupt)
			{
            test.Printf(_L("The volume was corrupt. Formatting...\r\n"));
            FormatLFFS(anFsSession,lffsDriveLetter);
        	}
        else if(r==KErrNotReady)
			{
            test.Printf(_L("The mount was not ready. Formatting...\r\n"));
            FormatLFFS(anFsSession,lffsDriveLetter);
        	}
        else
			{
            test.Printf(_L("The volume was mounted OK. Formatting...\r\n"));
            FormatLFFS(anFsSession,lffsDriveLetter); // ???
        	}
    	}
    else
		{
        test.Printf(_L("LFFS already mounted on drive %S\r\n"), &lffsDriveLetter);
    	}
    
    return(ETrue);
	}

GLDEF_C TBool IsTestingLFFS()
//
// Return ETrue if testing LFFS
//
	{
    return(LFFSTesting);
	}

GLDEF_C void TestingLFFS(TBool aSetting)
//
// Set whether testing LFFS or not
//
	{
    LFFSTesting=aSetting;
	}

GLDEF_C TInt GetDriveLFFS()
//
// Return the LFFS drive number
//
	{
    return(LFFSdriveNumber);
	}

GLDEF_C TBool IsSessionDriveLFFS(RFs& aFs,TChar& aDriveLetter)
	{
//
// Quick method of testing if session drive is LFFS
//	
	TBool isLffs;
	TFileName path;
	TInt r=aFs.SessionPath(path);
	test(r==KErrNone);
	TInt drv;
	r=RFs::CharToDrive(path[0],drv);
	test(r==KErrNone);

	aDriveLetter=path[0];
	isLffs=IsFileSystemLFFS(aFs,drv);
	if(isLffs)
		return(ETrue);

	// check if platform expects lffs to be mounted on default drive
	TInt lffsDrv = CheckLFFSDriveForPlatform();
	if (lffsDrv == KInvalidDriveLetter)
		{
		test.Printf(_L("IsSessionDriveLFFS: platform does not support lffs.\r\n"));
		isLffs = EFalse;
		}
	else
		{
		TChar curCh=path[0];
		curCh.LowerCase();

		TChar lffsCh;								// lffs drv ltr
		test((r = RFs::DriveToChar(lffsDrv, lffsCh)) == KErrNone);
		lffsCh.LowerCase();
		
		test.Printf(_L("IsSessionDriveLFFS: cur drv = \'%c\', lffs drv = \'%c\'.\n"), (TText) curCh, (TText) lffsCh);
		isLffs = ((TText) curCh) == ((TText) lffsCh);
		}

	return(isLffs);
	}

GLDEF_C TBool IsDefaultDriveLFFS()
//
// Quick method of testing if running on LFFS for non t_main based tests.
// 
	{
	// check if lffs mounted on default drive
	TBool isLffs;
	RFs fs;
	TInt r=fs.Connect();
	test(r==KErrNone);
	TFileName path;
	r=fs.SessionPath(path);
	test(r==KErrNone);
	TInt drv;
	r=TheFs.CharToDrive(path[0],drv);
	test(r==KErrNone);

	isLffs=IsFileSystemLFFS(fs,drv);
	fs.Close();
	if(isLffs)
		return(ETrue);

	// check if platform expects lffs to be mounted on default drive
	TInt lffsDrv = CheckLFFSDriveForPlatform();
	if (lffsDrv == KInvalidDriveLetter)
		{
		test.Printf(_L("IsCurrentDriveLFFS: platform does not support lffs.\r\n"));
		isLffs = EFalse;
		}
	else
		{
		TChar curCh=path[0];
		curCh.LowerCase();

		TChar lffsCh;								// lffs drv ltr
		test((r = RFs::DriveToChar(lffsDrv, lffsCh)) == KErrNone);
		lffsCh.LowerCase();
		
		test.Printf(_L("IsCurrentDriveLFFS: cur drv = \'%c\', lffs drv = \'%c\'.\n"), (TText) curCh, (TText) lffsCh);
		isLffs = ((TText) curCh) == ((TText) lffsCh);
		}

	return(isLffs);
	}

GLDEF_C TBool IsNamedDriveLFFS(RFs &aFsSession,TText aDrv)
//
// Quick method of testing if running on LFFS for non t_main based tests.
// 
	{
	TInt d;
	TInt r=RFs::CharToDrive(aDrv,d);
	test(r==KErrNone);
	return(IsFileSystemLFFS(aFsSession,d));
	}

GLDEF_C TInt GetLFFSControlModeSize()
//
// For LFFS, the media may not exhibit a contiguous data region. This is the case if the 
// Control Mode Size is non-zero.
//
	{
	TLocalDriveCapsV7 caps;		// V7 to allow for devices exhibiting Control Mode
	TPckg<TLocalDriveCapsV7> capsPckg(caps);
	TBusLocalDrive localDrive;
	TBool lffsMediaFound = EFalse;
	TBool dumBool = EFalse;		// Arbitrary if LFFS is mounted on non-removable media
	// Loop to find the local drive for LFFS - this is always of type EMediaFlash
	for(TInt drvNum=0; drvNum<KMaxLocalDrives; drvNum++)
		{
		TInt r=localDrive.Connect(drvNum,dumBool);
		if(r==KErrNotSupported)
			continue;		// Local drive not present
		test_KErrNone(r);
		r=localDrive.Caps(capsPckg);
		localDrive.Disconnect();
		if(r==KErrNotSupported||r==KErrNotReady)
			continue;		// Local drive not available
		test_KErrNone(r);
		if(capsPckg().iType==EMediaFlash)
			{
			lffsMediaFound=ETrue;
			break;
			}
		}
	if(!lffsMediaFound)
		{
		test.Printf(_L("GetLFFSControlModeSize: LFFS media not found !\n"));
		return KErrGeneral;
		}
	return (capsPckg().iControlModeSize);
	}
