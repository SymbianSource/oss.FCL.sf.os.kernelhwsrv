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
// f32test\bench\t_notify_perf.cpp
// 
//

#include "t_notify_perf.h"
#include "t_server.h"

RTest test(_L("Enhanced Notification Performance Test"));

extern void ClearTestPathL();
extern void SetTestPaths();
extern void CopyLogFilesL();
extern void DeleteLogFilesL();

enum TTestControl
    {
    ESingleSession  = 0x01,
    EMultiSession   = 0x02,
    EOtherTestCase  = 0x04,
    EPluginTest     = 0x08,
    ECopyLogs       = 0x10
    };

const TInt KTestControlMask = 0x07;

// Note: Format of command line argument
// t_notify_perf <Drive> <-s|-m|-o> [-p] [-cplg] <Number>
//
// <Drive>  The drive you want to run the test on
// -s       Execute single session test - PBASE-T_NOTIFY-2456
// -m       Execute multi sessions test - PBASE-T_NOTIFY-2457
// -o       Execute other edge use test - PBASE-T_NOTIFY-2458
// -p       Enable Mds plugin test, this does not take effect in multi session test
// -cplg    Copy the log files to MMC card after test finish, not work for emulator. MMC drive hard coded to be "D"
// <Number> The number of files you want to use in the tests, does not take effect in other esge use test   

// Function for parsing command line arguments for the test
LOCAL_C void ParseCmdArg(TUint16& aTestControl, TInt& aNumFiles)

    {
    TBuf<0x100> cmd;
    User::CommandLine(cmd);
    TLex lex(cmd);
    
    TPtrC token = lex.NextToken();
    TFileName thisfile = RProcess().FileName();
    if (token.MatchF(thisfile)==0)
        {
        token.Set(lex.NextToken());
        }

    if(token.Length()!=0)       
        {
        gDriveToTest=token[0];
        gDriveToTest.UpperCase();
        }
    else
        {
        gDriveToTest='C';
        }
    
    while (!lex.Eos())
        {
        token.Set(lex.NextToken());
        if (token.Compare(_L("-s")) == 0 || token.Compare(_L("-S")) == 0)
            {
            aTestControl |= ESingleSession;
            }
        else if (token.Compare(_L("-m")) == 0 || token.Compare(_L("-M")) == 0)
            {
            aTestControl |= EMultiSession;
            }
        else if (token.Compare(_L("-o")) == 0 || token.Compare(_L("-O")) == 0)
            {
            aTestControl |= EOtherTestCase;
            }
        else if (token.Compare(_L("-p")) == 0 || token.Compare(_L("-P")) == 0)
            {
            aTestControl |= EPluginTest;
            }
        else if (token.Compare(_L("-cplg")) == 0 || token.Compare(_L("-CPLG")) == 0)
            {
            aTestControl |= ECopyLogs;
            }
        else
            {
            TLex valArg(token);
            TInt r = valArg.Val(aNumFiles);
            if (r != KErrNone)
                {
                RDebug::Print(_L("Bad Argument: %S"), &token);
                test(r == KErrNone);
                }
            }
        }
    
    TInt mode = aTestControl & KTestControlMask;
    if ((mode != ESingleSession) && (mode != EMultiSession) && (mode != EOtherTestCase))
        {
        RDebug::Print(_L("Bad Argument: One and only one mode (-s/-m/-o) should be set."));
        test((mode == ESingleSession) || (mode == EMultiSession) || (mode == EOtherTestCase));
        }
    
    if ((aNumFiles <= 0) && ((mode == ESingleSession) || (mode == EMultiSession)))
        {
        RDebug::Print(_L("Bad Argument: number of files invalid"));
        test(aNumFiles > 0);
        }
    
    if ((aNumFiles > 0) && (mode == EOtherTestCase))
        RDebug::Print(_L("File numbers ignored..."));
    
    if ((mode == EMultiSession) && (aTestControl & EPluginTest))
        RDebug::Print(_L("-p option ignored..."));
    
    gLogPostFix.FillZ();
    gLogPostFix.Append('_');
    gLogPostFix.Append(gDriveToTest);
    
    if (mode == EOtherTestCase)
        gLogPostFix.Append(_L("_O"));
    
    if ((aTestControl & EPluginTest) && (mode != EMultiSession))
        gLogPostFix.Append(_L("_Plugin"));
    
    if (mode != EOtherTestCase)
        gLogPostFix.AppendFormat(_L("_%d"), aNumFiles);

    gLogPostFix.Append(_L(".log"));
    }

//---------------------------------------------
//! @SYMTestCaseID			PBASE-T_NOTIFY-2456
//! @SYMTestType 			PT
//! @SYMREQ 				PREQ1847
//! @SYMTestCaseDesc 		Performance Test – Single File Server Session
//! @SYMTestActions         Perform a series of file operations with different numbers with different notification
//!                         mechanism, measure the time taken and produce the log with the results
//! @SYMTestExpectedResults	Figures of performance
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------
LOCAL_C void SingleFileSessionTestL(TInt aNum, TBool aRunPlugin)
	{
	test.Start(_L("Performance Test - Single File Server Session, Test Preparation"));
	
	const TInt KNumClients = 1;
	ClearTestPathL();
	
	// ------------------ Files --------------------------
	
	test.Next(_L("Files, Single file session, no notification"));
	TTestSetting setting (aNum, 0, ENoNotify, KDefaultOpList);
	CTestExecutor testcase (setting);
	testcase.RunTestCaseL();

	test.Next(_L("Files, Single file session, enhanced notification - Change reporting enabled"));
	ClearTestPathL();
	setting.iOption = (EEnhanced|EReportChg|EBigBuffer);
	setting.iNumCli = KNumClients;
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Files, Single file session, original notification - Change reporting enabled"));
	ClearTestPathL();
	setting.iOption = (EOriginal|EReportChg);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();

	if (aRunPlugin)
	    {
        test.Next(_L("Files, Single file session, Plugin - Change reporting enabled"));
        ClearTestPathL();
        setting.iOption = (EPlugin|EReportChg);
        testcase.SetTestSetting(setting);
        testcase.RunTestCaseL();
	    }
	
	test.Next(_L("Files, Single file session, enhanced notification - Change reporting disabled"));
	ClearTestPathL();
	setting.iOption = (EEnhanced|EBigBuffer);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Files, Single file session, original notification - Change reporting disabled"));
	ClearTestPathL();
	setting.iOption = (EOriginal);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();

    if (aRunPlugin)
        {
        test.Next(_L("Files, Single file session, Plugin - Change reporting disabled"));
        ClearTestPathL();
        setting.iOption = (EPlugin);
        testcase.SetTestSetting(setting);
        testcase.RunTestCaseL();
        }
    
	test.Next(_L("Files, Single file session, enhanced notification - with long list of filters"));
	ClearTestPathL();
	setting.iOption = (EEnhanced|EReportChg|EBigBuffer|EBigFilter);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();

	// ------------------------- Directories -------------------------
	test.Next(_L("Dirs, Single file session, enhanced notification"));
	ClearTestPathL();
	setting.iOperationList = KDefaultOpListDir;
	setting.iOption = (EEnhanced|EReportChg|EBigBuffer);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Dirs, Single file session, original notification"));
	ClearTestPathL();
	setting.iOption = (EOriginal|EReportChg);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();

    if (aRunPlugin)
        {
        test.Next(_L("Dirs, Single file session, Plugin"));
        ClearTestPathL();
        setting.iOption = (EPlugin|EReportChg);
        testcase.SetTestSetting(setting);
        testcase.RunTestCaseL();
        }

	test.Next(_L("Test finishing - clearing test path"));
	
	ClearTestPathL();
	test.End();
	}

//---------------------------------------------
//! @SYMTestCaseID          PBASE-T_NOTIFY-2457
//! @SYMTestType            PT
//! @SYMREQ                 PREQ1847
//! @SYMTestCaseDesc        Performance Test – Multiple File Server Sessions
//! @SYMTestActions         Perform a series of file operations with different numbers with different notification
//!                         mechanism, create multiple notification threads to collect the notifications, measure 
//!                         the time taken and produce the log with the results
//! @SYMTestExpectedResults Figures of performance
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------
LOCAL_C void MultipleFileSessionTestL(TInt aNum)
    {
    test.Start(_L("Performance Test - Multi File Server Session, Test Preparation"));
    
    const TInt KNumClients = 4;
    ClearTestPathL();
    
    test.Next(_L("Multi file sessions, enhanced notification"));
    TTestSetting setting (aNum, KNumClients, (EEnhanced|EReportChg|EBigBuffer), KDefaultOpList);
    CTestExecutor testcase (setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Multi file sessions, original notification"));
    ClearTestPathL();
    setting.iOption = (EOriginal|EReportChg);
    testcase.SetTestSetting(setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Multi file sessions, enhanced notification - Lots of Filters"));
    ClearTestPathL();
    setting.iOption = (EEnhanced|EReportChg|EBigFilter|EBigBuffer);
    testcase.SetTestSetting(setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Multi file sessions, multi notifications Mode 1, enhanced notification"));
    ClearTestPathL();
    setting.iOption = (EEnhanced|EReportChg|EBigBuffer|EMultiNoti1);
    testcase.SetTestSetting(setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Multi file sessions, multi notifications Mode 2, enhanced notification"));
    ClearTestPathL();
    setting.iOption = (EEnhanced|EReportChg|EBigBuffer|EMultiNoti2);
    testcase.SetTestSetting(setting);
    testcase.RunTestCaseL();
    
    test.Next(_L("Test finishing - clearing test path"));
    
    ClearTestPathL();
    test.End();
    }

//---------------------------------------------
//! @SYMTestCaseID			PBASE-T_NOTIFY-2458
//! @SYMTestType 			PT
//! @SYMREQ 				PREQ1847
//! @SYMTestCaseDesc 		Performance Test – Test of other edge use cases
//! @SYMTestActions         Perform 1. Large number of changes in single file; 2. small changes in large number of files;
//!                         3. Mixed File operations; on each kind of notification mechanism, and measure the performance
//! @SYMTestExpectedResults	Figures of performance
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//---------------------------------------------
LOCAL_C void EdgeUseCaseTestL(TBool aRunPlugin)
	{
	test.Start(_L("Performance Test - Test of other edge use cases, Test Preparation"));
	
	const TInt KNumChanges = 1000;
	const TInt KNumMixed = 50;	// using a small number because the mixed operation test will perform (18 * KNumMixed) operations
	const TInt KNumClients = 1;
	ClearTestPathL();
	
	test.Next(_L("Single file session, enhanced notification, big number of changes on single file"));
	TTestSetting setting (KNumChanges, KNumClients, (EEnhanced|EReportChg|EBigBuffer), KManyChangesOpList);
	CTestExecutor testcase (setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Single file session, original notification, big number of changes on single file"));
	ClearTestPathL();
	setting.iOption = (EOriginal|EReportChg);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Single file session, enhanced notification, small changes on big number of files"));
	ClearTestPathL();
	setting.iOption = (EEnhanced|EReportChg|EBigBuffer);
	setting.iOperationList = KManyFilesOpList;
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Single file session, original notification, small changes on big number of files"));
	ClearTestPathL();
	setting.iOption = (EOriginal|EReportChg);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();	

	test.Next(_L("Single file session, enhanced notification, mixed file operations, big buffer"));
	ClearTestPathL();
	setting.iNumFiles = KNumMixed;
	setting.iOption = (EEnhanced|EReportChg|EBigBuffer);
	setting.iOperationList = KMixedOpTestList;
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Single file session, original notification, mixed file operations"));
	ClearTestPathL();
	setting.iOption = (EOriginal|EReportChg);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();

    if (aRunPlugin)
        {
        test.Next(_L("Single file session, Plugin, mixed file operations"));
        ClearTestPathL();
        setting.iOption = (EPlugin|EReportChg);
        testcase.SetTestSetting(setting);
        testcase.RunTestCaseL();
        }
	
	test.Next(_L("Single file session, enhanced notification, mixed file operations, small buffer"));
	ClearTestPathL();
	setting.iOption = (EEnhanced|EReportChg);
	testcase.SetTestSetting(setting);
	testcase.RunTestCaseL();
	
	test.Next(_L("Test finishing - clearing test path"));
	ClearTestPathL();
	test.End();
	}

// Entry Point
GLDEF_C void CallTestsL()
	{
	test.Start(_L("Enhanced Notification Performance Test - Initializing...")); 
	 
	gPerfMeasure = ETrue;
	
	TUint16 ctrl = 0;
	TInt num = 0;
	ParseCmdArg(ctrl, num);

	SetTestPaths();
	DeleteLogFilesL();
	
	if (ctrl & ESingleSession)
	    {
        test.Next(_L("Performance Test - Single File Server Session")); 
        SingleFileSessionTestL(num, (ctrl & EPluginTest));
	    }
	
	if (ctrl & EMultiSession)
	    {
        test.Next(_L("Performance Test - Multi File Server Session"));
        MultipleFileSessionTestL(num);
	    }
	
	if (ctrl & EOtherTestCase)
	    {
        test.Next(_L("Performance Test - Test of other edge use cases"));
        EdgeUseCaseTestL(ctrl & EPluginTest);
	    }

	if (ctrl & ECopyLogs)
	    {
#ifndef __WINSCW__
	    CopyLogFilesL();
#endif
	    }

	test.End();
	test.Close();
	}



