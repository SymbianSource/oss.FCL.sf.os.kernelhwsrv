// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <f32file.h>

#include "rusbhostmsdevice.h"
#include "rextfilesystem.h"
#include "tmslog.h"


_LIT(KFsNm, "elocal");

RExtFileSystem::RExtFileSystem()
    {
    __MSFNLOG
    }

RExtFileSystem::~RExtFileSystem()
    {
    __MSFNLOG
    }


void RExtFileSystem::OpenL()
    {
    __MSFNLOG
    RFs fs;
    User::LeaveIfError(fs.Connect());
    CleanupClosePushL(fs);

    TInt err;
    err = fs.AddFileSystem(KFsNm);
    if (err != KErrAlreadyExists)
        User::LeaveIfError(err);

    err = fs.AddFileSystem(_L("ELOCAL"));
    if (!(KErrAlreadyExists == err || KErrCorrupt == err))
        User::LeaveIfError(err);

    err = fs.AddProxyDrive(_L("usbhostms.pxy"));
    if (!(KErrAlreadyExists == err || KErrCorrupt == err))
        User::LeaveIfError(err);

    CleanupStack::PopAndDestroy(&fs);
    }


void RExtFileSystem::CloseL()
    {
    __MSFNLOG
    RFs fs;
    User::LeaveIfError(fs.Connect());
    CleanupClosePushL(fs);
    TInt err = fs.RemoveProxyDrive(_L("usbhostms"));
    CleanupStack::PopAndDestroy(&fs);
    }


void RExtFileSystem::MountL(RUsbHostMsDevice& aDevice,
                            TDriveNumber aDriveNumber,
                            TToken aToken,
                            TLun aLun)
    {
    __MSFNLOG

    TTime start;
    TTime end;

    start.HomeTime();

    RFs fs;
    User::LeaveIfError(fs.Connect());
    CleanupClosePushL(fs);

    TInt err;
	err = aDevice.MountLun(aLun, aDriveNumber);
	if (!(KErrAlreadyExists == err || KErrNotReady == err))
		{
	    __PRINT1(_L("** Error: MountLun returned %d **"), err);
        RDebug::Print(_L("** Error: MountLun returned %d **"), err);
        User::LeaveIfError(err);
		}

    CleanupStack::PopAndDestroy(&fs);

    end.HomeTime();

    TTimeIntervalSeconds timeTaken;
    end.SecondsFrom(start, timeTaken);
    TUint totalTime = timeTaken.Int();
    RDebug::Print(_L("Mount has taken %dHrs:%dmins:%dsecs\n"),
                  totalTime/3600,
                  (totalTime/60)%60,
                  totalTime%60);
    }


void RExtFileSystem::DismountL(RUsbHostMsDevice& aDevice, TDriveNumber aDriveNumber)
    {
    __MSFNLOG
    RFs fs;
    User::LeaveIfError(fs.Connect());
    __PRINT(_L("DismountFileSystem"));
	//TInt err = aDevice.DismountLun(aDriveNumber);
    aDevice.DismountLun(aDriveNumber);
    fs.Close();
    }


TDriveNumber RExtFileSystem::GetDriveL()
    {
    RFs fs;
    User::LeaveIfError(fs.Connect());
    TDriveList driveList;
    fs.DriveList(driveList);
    fs.Close();

    TInt drive;
    for (drive = EDriveG; drive <= EDriveZ; drive++)
    	{
        // Skip K drive which is reserved for LFFS but shows as being free
        if (drive == EDriveK)
            {
            continue;
            }
        if (driveList[drive] == 0)
            {
            break;
            }
        }

    if (drive > EDriveZ)
        {
        RDebug::Print(_L("####### NOT Found free drive"));
        User::Leave(KErrInUse);
        }

    __PRINT1(_L("Found free drive @ %d"), drive);
    RDebug::Print(_L("####### Found free drive @ %d"), drive);
    return static_cast<TDriveNumber>(drive);
    }
