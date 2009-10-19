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
#include <e32base.h>
#include <f32file.h>
#include <e32cons.h>
#include <e32debug.h>
#define __E32TEST_EXTENSION__
#include <e32test.h>

#include "cmsdrive.h"
#include "tmsprintdrive.h"
#include "ttestutils.h"
#include "cblockdevicetester.h"
#include "tmslog.h"

extern CMsDrive* msDrive;

RTest test(_L("T_MSRMB"));
RFs fsSession;

class TTests
    {
public:
    TTests(TInt aDriveNumber);

    void FullFormat();

    void tRemovableSet();
    void tRemovableClr();
    void RemountDrive();

private:
    TInt iDriveNumber;
    };

TTests::TTests(TInt aDriveNumber)
:   iDriveNumber(aDriveNumber)
    {
    }


void TTests::RemountDrive()
    {
    TDriveInfo info;
    const TUint KMediaRemountForceMediaChange = 0x00000001;

    TRequestStatus changeStatus;
    fsSession.NotifyChange(ENotifyAll, changeStatus);

    test.Printf(_L("Remounting the drive\n"));
    TInt err = fsSession.RemountDrive(iDriveNumber, NULL, KMediaRemountForceMediaChange);
    test(err == KErrNotReady || err == KErrNone);

    do
        {
        test.Printf(_L("Waiting for media change...\n"));
        User::WaitForRequest(changeStatus);

        err = fsSession.Drive(info, iDriveNumber);
        test.Printf(_L("Completed\n"));

        fsSession.NotifyChange(ENotifyAll, changeStatus);
        }
    while (err == KErrNotReady);
    fsSession.NotifyChangeCancel(changeStatus);
    }


void TTests::tRemovableSet()
    {
    test.Start(_L("tRemovableSet\n"));

    TDriveInfo driveInfo;
    TInt err = fsSession.Drive(driveInfo);
    test(err == KErrNone);

    test.Printf(_L("DriveInfo DriveAtt= %x\n"), driveInfo.iDriveAtt);

    test(driveInfo.iDriveAtt&KDriveAttRemovable != 0);
    test.End();
    }


void TTests::tRemovableClr()
    {
    test.Start(_L("tRemovableClr\n"));

    TDriveInfo driveInfo;
    TInt err = fsSession.Drive(driveInfo);
    test(err == KErrNone);

    test.Printf(_L("DriveInfo DriveAtt= %x\n"), driveInfo.iDriveAtt);

    test(driveInfo.iDriveAtt&KDriveAttRemovable != 0);
    test.End();
    }


void TTests::FullFormat()
    {
    TInt counter;
    RFormat format;
    test.Next(_L("Test EFullFormat"));

    TInt err = format.Open(fsSession, msDrive->GetSessionPath(), EFullFormat, counter);
    test(err == KErrNone);
    while (counter)
        {
        err = format.Next(counter);
        test(err == KErrNone);
        }
    format.Close();
    }



void CallTestsL()
    {
    TInt driveNumber = msDrive->DriveNumber();
    // Print drive info
    TRAPD(err, TMsPrintDrive::VolInfoL(driveNumber));
    err = err;

    TTests t(driveNumber);

    test.Printf(_L("Preparing target drive..."));
    //t.FullFormat();

    CWrPrTester* wpTester = CWrPrTester::NewL(driveNumber);
    CleanupStack::PushL(wpTester);
    test.Printf(_L("Target drive ready.\n"));

    test.Start(_L("REMOVABLE"));

    test.Next(_L("Test RMB=Set\n"));
    err = wpTester->SetRemovableL();
    test(err == KErrNone);
    t.RemountDrive();
    t.tRemovableSet();

    test.Next(_L("Test RMB=Clr\n"));
    err = wpTester->ClrRemovableL();
    test(err == KErrNone);
    t.RemountDrive();
    t.tRemovableClr();
    test.End();

    CleanupStack::PopAndDestroy(wpTester);
    }
