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
// f32test\filesystem\fat\t_scn32dr2.cpp
//

#define __E32TEST_EXTENSION__

#include <f32file.h>
#include <e32test.h>
#include "t_server.h"

#include "fat_utils.h"
using namespace Fat_Test_Utils;



/* Tests rugged fat file system. Power failure is simulated by failure of write
operation in TDriver class. See t_tscan.cpp for fuller description.
Tests drive set in the default path for epoc platforms and x: for WINS.*/

GLDEF_D RTest test(_L("T_SCN32DR2"));
GLDEF_D TFileName TheDrive=_L("?:\\");

GLREF_D TInt TheFunctionNumber;
GLREF_D TInt TheOpNumber;
GLREF_D TInt TheFailCount;
GLREF_D TBool IsReset;
GLREF_D RFs TheFs;
GLREF_D TFileName TestExeName;
GLREF_D TFileName StartupExeName;
GLREF_D TFileName LogFileName;

#ifdef _DEBUG
const TInt KControlIoRuggedOn=2;
const TInt KControlIoRuggedOff=3;
const TInt KControlIoIsRugged=4;
#endif
GLREF_D TInt WriteFailValue;

GLREF_C void ReadLogFile();
GLREF_C void DoTests();

GLDEF_C void CallTestsL()
//
// Do all tests
//
    {
    if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
        StartupExeName=_L("?:\\SYS\\BIN\\ESHELL.EXE");
    else
        StartupExeName=_L("?:\\SYSTEM\\BIN\\ESHELL.EXE");


#ifndef _DEBUG
    test.Printf(_L("Error: Only debug builds supported\n"));
    return;
#else

    const TInt KWriteFailStd=-100; // error -100 returned from write fail

    TInt gDriveNumber;

    TInt r = TheFs.CharToDrive( gSessionPath[0], gDriveNumber );
    test_KErrNone(r);

    //-- set up console output
    Fat_Test_Utils::SetConsole(test.Console());

    //-- print drive information
    PrintDrvInfo(TheFs, gDriveNumber);

    if (!Is_Fat(TheFs, gDriveNumber))
        {
        test.Printf(_L("CallTestsL: Skipped: test requires FAT filesystem\n"));
        return;
        }

    TheFunctionNumber=0;
    TheOpNumber=0;
    TheFailCount=0;
    IsReset=EFalse;
    WriteFailValue=KWriteFailStd;

    // ensure that fat filing system is rugged
    TUint8 oldFsys;
    TPtr8 pRugged(&oldFsys,1,1);
    r=TheFs.ControlIo(gDriveNumber,KControlIoIsRugged,pRugged);
    test_KErrNone(r);
    if(oldFsys==0)
        {
        r=TheFs.ControlIo(gDriveNumber,KControlIoRuggedOn);
        test_KErrNone(r);
        }
    DoTests();
    // if nec, set filing system back to !rugged
    if(oldFsys==0)
        {
        r=TheFs.ControlIo(gDriveNumber,KControlIoRuggedOff);
        test_KErrNone(r);
        }

    return;
#endif
    }



