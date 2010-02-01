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

#include "tmsprintdrive.h"
#include "tmslog.h"
#include "cmsdrive.h"


RTest test(_L("T_MSMAN"));
RFs fsSession;

extern CMsDrive msDrive;

void DriveTestL()
    {
    test.Start(_L("Check USB drive attributes\n"));
    TInt driveNumber = msDrive.DriveNumber();

    // Check drive Info is USB Mass Storage
    TDriveInfo driveInfo;
    fsSession.Drive(driveInfo, driveNumber);

    test_Equal(driveInfo.iConnectionBusType, EConnectionBusUsb);
    test_Value(driveInfo.iDriveAtt & KDriveAttExternal, KDriveAttExternal);
    test.End();
    }


void CallTestsL()
    {
    DriveTestL();
    _LIT(KTxtEnd, "Press key to end session\n");
    test.Printf(KTxtEnd);
    test.Getch();
    }
