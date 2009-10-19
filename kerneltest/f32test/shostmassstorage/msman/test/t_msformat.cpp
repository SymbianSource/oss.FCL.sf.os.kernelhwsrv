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
#include <e32test.h>

#include "cmsdrive.h"
#include "tmsprintdrive.h"
#include "tmslog.h"

extern CMsDrive* msDrive;

RTest test(_L("T_MSFORMAT"));
RFs fsSession;



void DisplayVolumeInfo()
    {
    TVolumeInfo volInfo;
    TInt err = fsSession.Volume(volInfo);
    test(err == KErrNone);

    if (volInfo.iSize-volInfo.iFree != 0)
        {
        test.Printf(_L("Memory 'in use' after a full format = %ld\n"),(volInfo.iSize-volInfo.iFree));
        test.Printf(_L("volInfo.iSize = %ld\n"),volInfo.iSize);
        test.Printf(_L("volInfo.iFree = %ld\n"),volInfo.iFree);
        }

    test.Next(_L("CheckDisk"));
    err = fsSession.CheckDisk(msDrive->GetSessionPath());
    test(err == KErrNone);

    test.Next(_L("ScanDrive"));
    err = fsSession.ScanDrive(msDrive->GetSessionPath());
    test(err == KErrNone);
    }


void TestFullFormat()
    {
    test.Start(_L("Test EFullFormat"));
    TInt counter;
    RFormat format;

    TInt err = format.Open(fsSession, msDrive->GetSessionPath(), EFullFormat, counter);
    test(err == KErrNone);
    while(counter)
        {
        test.Printf(_L("."));
        err = format.Next(counter);
        test(err == KErrNone);
        }
    test.Printf(_L("\n\r"));

    format.Close();
    DisplayVolumeInfo();
    test.End();
    }


void TestQuickFormat()
    {
    test.Start(_L("Test EQuickFormat"));
    TInt counter;
    RFormat format;

    TInt err = format.Open(fsSession, msDrive->GetSessionPath(), EQuickFormat, counter);
    test(err == KErrNone);
    while(counter)
        {
        err = format.Next(counter);
        test(err == KErrNone);
        }
    format.Close();
    DisplayVolumeInfo();
    test.End();
    }


void CallTestsL()
    {
    TestFullFormat();
    TestQuickFormat();
    }
