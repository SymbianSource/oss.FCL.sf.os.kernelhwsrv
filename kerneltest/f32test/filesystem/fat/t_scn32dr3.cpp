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
// f32test\scndrv\t_scn32dr3.cpp
//
//

#define	__E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <e32svr.h>
#include "t_server.h"

#include "fat_utils.h"
using namespace Fat_Test_Utils;


/* Tests rugged fat filing system for epoc platforms. Copies \t_scn32dr3.exe to
\sys\bin\eshell.exe to enable the test to carry on after a reset.
See t_tscan for a fuller description of the tests carried out. Good idea to
disable crash debugger via \e32\inc\m32std.h. Drive to be tested is selected at
the command line. Also useful to disable beep in \e32\kpepoc\kp_ini.cpp to
avoid excessive noise.*/

GLDEF_D RTest test(_L("T_SCN32DR3"));

GLREF_D TInt TheFunctionNumber;
GLREF_D TInt TheOpNumber;
GLREF_D TInt TheFailCount;
GLREF_D TBool IsReset;
GLREF_D RFs TheFs;
GLREF_D TFileName TestExeName;
GLREF_D TFileName StartupExeName;
GLREF_D TFileName LogFileName;

#if defined( _DEBUG) && !defined(__WINS__)
const TInt KControlIoRuggedOn=2;
const TInt KControlIoRuggedOff=3;
const TInt KControlIoIsRugged=4;
#endif
GLREF_D TInt WriteFailValue;
GLREF_C void ReadLogFile();
GLREF_C void DoTests();

#if defined(_DEBUG)
LOCAL_C void OverrideEShell(void)
//
// Copies the executable file as the eshell so that it is automatically run
//
    {
    test.Next(_L("OverideEShell"));
    RProcess myProc;
    if (myProc.FileName().CompareF(StartupExeName) == 0)
        test.Printf(_L("OverrideEShell: running as eshell\n"));
    else
        {
        RFile logFile;
        TInt r;
        TFileName tempDirName=_L("");
        // Copy over this executable and create a progress file.
        test.Printf(_L("Copying %S to %S\n"), &TestExeName, &StartupExeName);
        if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
            tempDirName = _L("?:\\SYS\\BIN\\");
        else
            tempDirName = _L("?:\\SYSTEM\\BIN\\");
        tempDirName[0] = gSessionPath[0];
        r=TheFs.MkDirAll(tempDirName);
        test_Value(r, r == KErrNone||r==KErrAlreadyExists);
        CFileMan* fileMan=NULL;
        TRAP(r,fileMan = CFileMan::NewL(TheFs));
        test_KErrNone(r);
        //Copy the test from Z drive.
        TFileName temp=_L("Z:\\SYS\\BIN\\T_SCN32DR3.EXE");
        r = fileMan->Copy(temp, TestExeName, CFileMan::EOverWrite);
        test_KErrNone(r);
        r = fileMan->Copy(TestExeName, StartupExeName, CFileMan::EOverWrite);
        test_KErrNone(r);
        //Mask read attribute. Fix for DEF081323
        r = fileMan->Attribs(StartupExeName, 0, KEntryAttReadOnly, 0);
        test_KErrNone(r);
        r = fileMan->Attribs(TestExeName, 0, KEntryAttReadOnly, 0);
        test_KErrNone(r);
        r = logFile.Replace(TheFs,LogFileName,EFileShareExclusive|EFileWrite);
        test_KErrNone(r);
        logFile.Close();
        delete fileMan;
        }
    }
#endif

GLDEF_C void CallTestsL()
//
//
//
    {
    if(PlatSec::ConfigSetting(PlatSec::EPlatSecEnforceSysBin))
        StartupExeName=_L("?:\\SYS\\BIN\\ESHELL.EXE");
    else
        StartupExeName=_L("?:\\SYSTEM\\BIN\\ESHELL.EXE");
    //RFs::ControlIo only supported in debug build
#ifndef _DEBUG
    test.Printf(_L("Error: Supported only debug testing\n"));
    return;
#else
#if defined(__WINS__)
    test.Printf(_L("WINS not tested\n"));
    return;
#else
    const TInt KWriteFailReset=-99;         // soft reset after write fail

    TInt r;
    TestExeName[0]=StartupExeName[0]=LogFileName[0]=gSessionPath[0];
    // ensure file system is rugged
    TUint8 oldFsys;
    TPtr8 pRugged(&oldFsys,1,1);
    r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoIsRugged,pRugged);
    test_KErrNone(r);
    if(oldFsys==0)
        {
        r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoRuggedOn);
        test_KErrNone(r);
        }
    TheFunctionNumber=0;
    TheOpNumber=0;
    TheFailCount=0;
    IsReset=ETrue;
    WriteFailValue=KWriteFailReset;
    test.Printf(_L("IsReset=%d\n"),IsReset);
    OverrideEShell();
    ReadLogFile();
    r=TheFs.ScanDrive(gSessionPath);
    test_KErrNone(r);
    r=TheFs.CheckDisk(gSessionPath);
    test_KErrNone(r);
    DoTests();
    r=TheFs.Delete(LogFileName);
    test_KErrNone(r);
    r=TheFs.Delete(StartupExeName);
    test_KErrNone(r);
    // return file system to original state
    if(oldFsys==0)
        r=TheFs.ControlIo(gSessionPath[0]-'A',KControlIoRuggedOff);
    UserSvr::ResetMachine(EStartupWarmReset);
    return;
#endif
#endif
    }


