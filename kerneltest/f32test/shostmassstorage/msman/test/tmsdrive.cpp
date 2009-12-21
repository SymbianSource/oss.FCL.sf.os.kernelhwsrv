// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//


#include <e32def.h>
#include <e32cmn.h>
#include <f32file.h>
#include <e32test.h>

#include "tmsprintdrive.h"
#include "tmsdrive.h"

extern RTest test;
extern RFs fsSession;


void TMsDrive::GetUsbDeviceL()
    {
    TDriveList drivelist1;
    TDriveList drivelist2;
    TChar driveLetter;
    TInt driveNumber;

    User::LeaveIfError(fsSession.DriveList(drivelist1));
    for (driveNumber = EDriveA; driveNumber <= EDriveZ; driveNumber++)
        {
        if (drivelist1[driveNumber])
            {
            User::LeaveIfError(fsSession.DriveToChar(driveNumber, driveLetter));
            }
        }

    TRequestStatus status;
    fsSession.NotifyChange(ENotifyAll, status);

    test.Printf(_L("Waiting for File System change notifcation...\r\n"));
    User::WaitForRequest(status);
    test.Printf(_L("NotifyChange status=%d\r\n"), status.Int());

    User::LeaveIfError(fsSession.DriveList(drivelist2));
    for (driveNumber = EDriveA; driveNumber <= EDriveZ; driveNumber++)
        {
        if (drivelist2[driveNumber])
            {
            User::LeaveIfError(fsSession.DriveToChar(driveNumber,driveLetter));
            }
        }

    for (driveNumber = 0; driveNumber < drivelist1.Length(); driveNumber++)
        {
        if (drivelist1[driveNumber] != drivelist2[driveNumber])
            {
            break;
            }
        }


    User::LeaveIfError(fsSession.DriveToChar(driveNumber,driveLetter));
    test.Printf(_L("Drive mounted on %c (0x%02x)"), TUint(driveLetter), drivelist2[driveNumber]);

    TRAPD(err, TMsPrintDrive::VolInfoL(driveNumber));
    // ignore error
    err = err;
    iDriveNumber = driveNumber;
    }


void TMsDrive::SetSessionPathL()
	{
    TChar driveLetter;
    User::LeaveIfError(fsSession.DriveToChar(iDriveNumber, driveLetter));
    TFileName path;

    _LIT(KPath, "%c:\\");
    path.Format(KPath, TUint(driveLetter));

	TInt err = fsSession.SetSessionPath(path);
	test(err == KErrNone);
	err = fsSession.SessionPath(iSessionPath);
	test(err == KErrNone);
    _LIT(KSession,"Session path for fsSession is %S\n");
    test.Printf(KSession, &iSessionPath);
	}



const TFileName& TMsDrive::GetSessionPath() const
    {
    return iSessionPath;
    }

TBool TMsDrive::DrivePresent() const
    {
    TDriveInfo driveInfo;
    fsSession.Drive(driveInfo, iDriveNumber);
    return driveInfo.iType == EMediaNotPresent ? EFalse : ETrue;
    }


