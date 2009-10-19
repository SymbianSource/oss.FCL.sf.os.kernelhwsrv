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
#include "tmslog.h"

extern CMsDrive* msDrive;

RTest test(_L("T_MSSUSPEND"));
RFs fsSession;


class TTestMsSuspend
    {
public:
    void t1L();

private:
    void DrvWrite();
    };


void TTestMsSuspend::t1L()
    {
    test.Start(_L("Test"));
    test.Next(_L("Test1"));
    TTestTimer iTimer;
    // #1
    DrvWrite();
    // wait for suspend
    test.Printf(_L("Waiting for host to suspend..."));
    iTimer.Start();
    TBool usbActive = TTestUtils::WaitForConnectionStateEventL();
    test(usbActive == EFalse);
    iTimer.End();

    // #2
    // Resume device by accessing drive
    DrvWrite();
    // wait for suspend
    test.Printf(_L("Waiting for host to suspend..."));
    iTimer.Start();
    usbActive = TTestUtils::WaitForConnectionStateEventL();
    iTimer.End();
    test(usbActive == EFalse);

    // #3
    // Resume device by accessing drive
    DrvWrite();
    // wait for suspend
    test.Printf(_L("Waiting for host to suspend..."));
    iTimer.Start();
    usbActive = TTestUtils::WaitForConnectionStateEventL();
    iTimer.End();
    test(usbActive == EFalse);

    // #4
    // Resume device by accessing drive
    DrvWrite();
    // wait for suspend
    test.Printf(_L("Waiting for host to suspend..."));
    iTimer.Start();
    usbActive = TTestUtils::WaitForConnectionStateEventL();
    iTimer.End();
    test(usbActive == EFalse);
    test.End();
    }


void TTestMsSuspend::DrvWrite()
    {
    RFile file;

    static const TInt KFileSize = 1000;
    _LIT(KTxtControlFile, "ControlFile.txt");
    static const TChar KFillChar = 'x';

    TBuf8<KFileSize> testData;
    testData.Fill(KFillChar, KFileSize);

    // write control file
    TInt err = file.Replace(fsSession, KTxtControlFile, EFileStream);
    test(err == KErrNone);

    err = file.Write(testData);
    test(err == KErrNone);

    file.Close();

    // delete the file
    err = fsSession.Delete(KTxtControlFile);
    test(err == KErrNone);
    }


void CallTestsL()
    {
    TInt driveNumber = msDrive->DriveNumber();
    // Print drive info
    TRAPD(err, TMsPrintDrive::VolInfoL(driveNumber));
    err = err;
    TTestMsSuspend t;
    t.t1L();

    }
