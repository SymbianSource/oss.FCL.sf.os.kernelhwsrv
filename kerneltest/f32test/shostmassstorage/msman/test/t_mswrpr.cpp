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

RTest test(_L("T_MSWRPR"));
RFs fsSession;



class TTests
    {
public:
    TTests(TInt aDriveNumber);

    void FullFormat();
    void tReadOnly();
    void tReadWrite();
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
        test.Printf(_L("Completed.\n"));

        fsSession.NotifyChange(ENotifyAll, changeStatus);
        }
    while (err == KErrNotReady);
    fsSession.NotifyChangeCancel(changeStatus);
    }

void TTests::tReadOnly()
    {
    test.Start(_L("tReadOnly\n"));

    TDriveInfo driveInfo;
    TInt err = fsSession.Drive(driveInfo);
    test(err == KErrNone);

    test.Printf(_L("DriveInfo MediaAtt= 0x%x\n"), driveInfo.iMediaAtt);

    test((driveInfo.iMediaAtt&KMediaAttWriteProtected) != 0);
    test.End();
    }


void TTests::tReadWrite()
    {
    test.Start(_L("tReadWrite\n"));

    TDriveInfo driveInfo;
    TInt err = fsSession.Drive(driveInfo);
    test(err == KErrNone);

    test.Printf(_L("DriveInfo MediaAtt = %x %x\n"), driveInfo.iMediaAtt, driveInfo.iMediaAtt&KMediaAttWriteProtected);
    test((driveInfo.iMediaAtt&KMediaAttWriteProtected) == 0);



    test.End();
    }


void TTests::FullFormat()
    {
    TInt counter;
    RFormat format;
    test.Next(_L("Test EFullFormat\n"));

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

    test.Start(_L("**** WRITE PROTECT ****"));

    t.RemountDrive();
    t.tReadWrite();
    err = wpTester->WriteReadTestL();
    test(err == KErrNone);

    test.Next(_L("**** Test WP=Set ****"));
    TRAP(err, wpTester->SetWriteProtectL());
    test(err == KErrNone);
    t.RemountDrive();
    t.tReadOnly();
    err = wpTester->WriteReadTestL();
    test(err == KErrAccessDenied);

    test.Next(_L("**** Test WP=Clr ****"));
    TRAP(err, wpTester->ClrWriteProtectL());
    test(err == KErrNone);
    t.RemountDrive();
    t.tReadWrite();
    err = wpTester->WriteReadTestL();
    test(err == KErrNone);

    test.End();

    CleanupStack::PopAndDestroy(wpTester);
    }
