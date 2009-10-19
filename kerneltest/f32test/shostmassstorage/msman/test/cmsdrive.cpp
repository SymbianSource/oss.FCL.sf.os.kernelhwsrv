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


#include <e32def.h>
#include <e32cmn.h>
#include <f32file.h>
#include <e32test.h>

#include "tmsprintdrive.h"
#include "cmsdrive.h"

extern RTest test;
extern RFs fsSession;

#define __CMSDRIVE_DEBUG

CMsDrive* CMsDrive::NewL()
    {
	CMsDrive* r = new (ELeave) CMsDrive();
	CleanupStack::PushL(r);

	r->ConstructL();
	CleanupStack::Pop();
	return r;
    }


void CMsDrive::ConstructL()
    {
    }


CMsDrive::CMsDrive()
    {
    GetInitialDriveMapL();
    }


CMsDrive::~CMsDrive()
    {
    }


void CMsDrive::GetInitialDriveMapL()
    {
    TChar driveLetter;

    User::LeaveIfError(fsSession.DriveList(iOriginalDrivelist));

#ifdef __CMSDRIVE_DEBUG
    for (TInt driveNumber = EDriveA; driveNumber <= EDriveZ; driveNumber++)
        {
        if (iOriginalDrivelist[driveNumber])
            {
            User::LeaveIfError(fsSession.DriveToChar(driveNumber, driveLetter));
            RDebug::Printf("> %d %c", driveNumber, (char)driveLetter);
            }
        }
#endif
    }


TBool CMsDrive::GetUsbDeviceL()
    {
    TChar driveLetter;
    TInt driveNumber;

    User::LeaveIfError(fsSession.DriveList(iMsDrivelist));
#ifdef __CMSDRIVE_DEBUG
    for (driveNumber = EDriveA; driveNumber <= EDriveZ; driveNumber++)
        {
        if (iMsDrivelist[driveNumber])
            {
            User::LeaveIfError(fsSession.DriveToChar(driveNumber,driveLetter));
            RDebug::Printf("< %d %c", driveNumber, (char)driveLetter);
            }
        }
#endif

    const TInt maxDrives = iOriginalDrivelist.Length();
    for (driveNumber = 0; driveNumber < maxDrives; driveNumber++)
        {
        if (iOriginalDrivelist[driveNumber] != iMsDrivelist[driveNumber])
            {
            break;
            }
        }

    if (driveNumber == maxDrives)
        {
        return EFalse;
        }

    User::LeaveIfError(fsSession.DriveToChar(driveNumber,driveLetter));
    test.Printf(_L("Drive mounted on %c (0x%02x)"), TUint(driveLetter), iMsDrivelist[driveNumber]);

    TRAPD(err, TMsPrintDrive::VolInfoL(driveNumber));
    // ignore error
    err = err;
    iDriveNumber = driveNumber;
    return ETrue;
    }


void CMsDrive::SetSessionPathL()
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



const TFileName& CMsDrive::GetSessionPath() const
    {
    return iSessionPath;
    }

TBool CMsDrive::DrivePresent() const
    {
    TDriveInfo driveInfo;
    fsSession.Drive(driveInfo, iDriveNumber);
    return driveInfo.iType == EMediaNotPresent ? EFalse : ETrue;
    }


