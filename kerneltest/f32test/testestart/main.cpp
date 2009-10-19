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
// f32\estart\estart.cpp
// 
//

//! @SYMTestCaseID FSBASE/ESTART/PREQ808/001
//! @SYMTestType CT
//! @SYMTestCaseDesc Test start-up mode property's availability and validity
//! @SYMPREQ 808
//! @SYMTestStatus Implemented
//! @SYMTestActions Read and write the property.
//! @SYMTestExpectedResults Pass read operation and fail write operation if no correct permission.
//! @SYMTestPriority Low
//! @SYMAuthor Ying Shi
//! @SYMCreationDate 03/11/2004
//! @See Estart component

#include <e32test.h>
#include <e32property.h>
#include <e32uid.h>
#include <e32debug.h>
#include <f32file.h>
#include <f32file_private.h>

RTest gTest(_L("T_StartupMode"));
RFs gRFs;

void DoTest()
    {
    TInt r;
    gTest.Next(_L("Read startup mode"));
    TInt sysStartupMode;
    r = RProperty::Get(KUidSystemCategory, KSystemStartupModeKey, sysStartupMode);
    gTest(r == KErrNone);       // Read operation should be successful
    RDebug::Printf("System startup mode: %d", sysStartupMode);

    gTest.Next(_L("write startup mode"));
    r = RProperty::Set(KUidSystemCategory, KSystemStartupModeKey, 1);
    if (PlatSec::ConfigSetting(PlatSec::EPlatSecEnforcement))
        gTest(r == KErrPermissionDenied);   // Should have no permission to write
    else
        gTest(r == KErrNone);   // Should have no permission to write
    }

void TestPlatSec()
    {
    TInt r = gRFs.SetStartupConfiguration(ELoaderPriority, (TAny*)EPriorityNormal, 0);
    gTest(r == KErrPermissionDenied);
    }

TInt E32Main()
    {
    gTest.Title();
    gTest.Start(_L("Start-up mode test sets"));
    gTest.SetLogged(ETrue);

    TInt r = gRFs.Connect();
    gTest(r == KErrNone);

    DoTest();

    TestPlatSec();

    gRFs.Close();

    gTest.End();
    gTest.Close();

    return 0;
    }
