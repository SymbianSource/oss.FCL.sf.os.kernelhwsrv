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
// f32test\server\t_notify_mfs.cpp
// 
//

#include "t_notify_perf.h"

RTest test(_L("T_NOTIFY_MFS - Multi File Session Test For Enhanced Notification"));

extern void ClearTestPathL();
extern void SetTestPaths();
extern void CopyLogFilesL();
extern void DeleteLogFilesL();

//---------------------------------------------
//! @SYMTestCaseID          PBASE-T_NOTIFY-2457
//! @SYMTestType            PT/UT
//! @SYMREQ                 PREQ1847
//! @SYMTestCaseDesc        Performance Test – Multiple File Server Sessions, this is only a part of the test case.
//!                         It is executed as a functional test here. This test case is also run as performance test in t_notify_perf 
//! @SYMTestActions         Perform a series of file operations, create multiple notification threads to collect the notifications.                        
//! @SYMTestExpectedResults No notifications are missed
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------
LOCAL_C void MultipleFileSessionTestL()
    {
    test.Start(_L("T_NOTIFY_MFS - Test Preparation"));
    
    gPerfMeasure = EFalse;
    SetTestPaths();
    DeleteLogFilesL();  // Does not write log file but need create the log folder for test stopper to use
    
    const TInt KNumFiles = 10;
    const TInt KNumClients = 4;
    
    test.Next(_L("Single Notification on all clients"));
    ClearTestPathL();
    TTestSetting setting (KNumFiles, KNumClients, (EEnhanced|EBigBuffer), KDefaultOpList);
    CTestExecutor testcase (setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Multi notifications Mode 1, enhanced notification"));
    ClearTestPathL();
    setting.iOption = (EEnhanced|EBigBuffer|EMultiNoti1);
    testcase.SetTestSetting(setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Multi notifications Mode 2, enhanced notification"));
    ClearTestPathL();
    setting.iOption = (EEnhanced|EBigBuffer|EMultiNoti2);
    testcase.SetTestSetting(setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Test finishing - clearing test path"));
    
    ClearTestPathL();
    test.End();
    }

// Entry Point
GLDEF_C void CallTestsL()
    {
    MultipleFileSessionTestL();
    }



