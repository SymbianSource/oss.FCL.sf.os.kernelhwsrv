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

#ifdef MSDC_TESTMODE

#include <e32cmn.h>
#include <e32def.h>
#include <e32des8.h>

#include <e32debug.h>

#include "debug.h"
#include "testman.h"


TTestParser::TTestParser()
:   iTestCase(ETestCaseNotSet),
    iEnable(EFalse),
    iSenseError(ETestSenseErrorNoSense),
    iPhaseErrorEnabled(EFalse),
    iTestCounter(0),
    iWriteProtect(EFalse),
    iRemovable(ETrue)
    {
    }

TTestParser::TTestCase TTestParser::TestCase() const
    {
    TTestCase testCase = iTestCase;

    return testCase;
    }

void TTestParser::ClrTestCase()
    {
    // clear testcase
    iTestCase = ETestCaseNotSet;
    iEnable = EFalse;
    __TESTMODEPRINT("Test condition cleared");
    }

void TTestParser::DecTestCounter()
    {
    if (iTestCounter > 0)
        {
        __TESTMODEPRINT1("TestCounter=%d", iTestCounter);
        iTestCounter--;
        }
    }


TBool TTestParser::DoutSearch(TPtrC8 aBuf)
    {
    /*
    RDebug::Printf("%x - %x %x %x %x %x %x %x %x %x...",
                   aBuf.Length(),
                   aBuf[0],aBuf[1], aBuf[2], aBuf[3],
                   aBuf[4],aBuf[5], aBuf[6], aBuf[7],
                   aBuf[8]);
    */
    TInt pos = aBuf.Match(KTxtTestCasePreamble);
    if (pos == KErrNotFound)
        {
        return EFalse;
        }

    TPtrC8 ptr = aBuf.Mid(pos + KTestCasePreambleLen);

    if (ptr.Length() >= 1)
        {
        TChar key = ptr[0];
        TPtrC8 testData = ptr.Mid(1);
        switch (key)
            {
            case KKeyTestCase:
                ParseTestCase(testData);
                break;
            case KKeyTestEnable:
                ParseTestEnable(testData);
                break;
            case KKeyTestSenseError:
                ParseTestSenseError(testData);
                break;
            case KKeyTestConfig:
                ParseTestConfig(testData);
                break;
            default:
                break;
            }
        }

    return ETrue;
    }


TBool TTestParser::DInSearch(TPtrC8 aBuf)
    {
    /*
    RDebug::Printf("DInSearch %x - %x %x %x %x %x %x %x %x %x...",
                   aBuf.Length(),
                   aBuf[0],aBuf[1], aBuf[2], aBuf[3],
                   aBuf[4],aBuf[5], aBuf[6], aBuf[7],
                   aBuf[8]);
    */
    TInt pos = aBuf.Match(KTxtTestCaseDinPreamble);
    if (pos == KErrNotFound)
        {
        return EFalse;
        }

    TPtrC8 ptr = aBuf.Mid(pos + KTestCasePreambleLen);

    if (ptr.Length() >= 1)
        {
        TChar key = ptr[0];
        TPtrC8 testData = ptr.Mid(1);
        switch (key)
            {
            case KKeyTestConfig:
                ParseTestConfig(testData);
                break;
            default:
                break;
            }
        }

    return ETrue;
    }


void TTestParser::ParseTestCase(const TDesC8& aTestData)
    {
    if (aTestData.Length() < 1)
        iTestCase = ETestCaseNotSet;

    iTestCase = static_cast<TTestCase>(aTestData[0]);
    iEnable = EFalse;

    // print test case to debug port
    TBuf<100>testCaseStr;
    _LIT(KTestCaseNotSet, "TestCaseNotSet");
    _LIT(KTestCaseTagMismatch, "TestCaseTagMismatch");
    _LIT(KTestCaseInvalidSignature, "TestCaseInvalidSignature");

    _LIT(KTestCaseNoDataStallCsw , "TestCaseNoDataStallCsw");
    _LIT(KTestCaseNoDataPhaseError, "TestCaseNoDataPhaseError");

    _LIT(KTestCaseDoStallCsw, "TestCaseDoStallCsw");
    _LIT(KTestCaseDoStallData, "TestCaseDoStallData");
    _LIT(KTestCaseDoPhaseError, "TestCaseDoPhaseError");
    _LIT(KTestCaseDoResidue , "TestCaseDoResidue");

    _LIT(KTestCaseDiStallCsw, "TestCaseDiStallCsw");
    _LIT(KTestCaseDiStallData, "TestCaseDiStallData");
    _LIT(KTestCaseDiPhaseError, "TestCaseDiPhaseError");
    _LIT(KTestCaseDiResidue , "TestCaseDiResidue");

    switch (iTestCase)
        {
        case ETestCaseTagMismatch:
            testCaseStr.Copy(KTestCaseTagMismatch);
            break;
        case ETestCaseInvalidSignature:
            testCaseStr.Copy(KTestCaseInvalidSignature);
            break;

        case ETestCaseNoDataStallCsw:
            testCaseStr.Copy(KTestCaseNoDataStallCsw);
            break;
        case ETestCaseNoDataPhaseError:
            testCaseStr.Copy(KTestCaseNoDataPhaseError);
            break;

        case ETestCaseDoStallCsw:
            testCaseStr.Copy(KTestCaseDoStallCsw);
            break;
        case ETestCaseDoStallData:
            testCaseStr.Copy(KTestCaseDoStallData);
            break;
        case ETestCaseDoPhaseError:
            testCaseStr.Copy(KTestCaseDoPhaseError);
            break;
        case ETestCaseDoResidue:
            iTestCounter = 4;
            testCaseStr.Copy(KTestCaseDoResidue);
            break;

        case ETestCaseDiStallCsw:
            testCaseStr.Copy(KTestCaseDiStallCsw);
            break;
        case ETestCaseDiStallData:
            testCaseStr.Copy(KTestCaseDiStallData);
            break;
        case ETestCaseDiPhaseError:
            testCaseStr.Copy(KTestCaseDiPhaseError);
            break;
        case ETestCaseDiResidue:
            iTestCounter = 4;
            testCaseStr.Copy(KTestCaseDiResidue);
            break;

        case ETestCaseNotSet:
        default:
            testCaseStr.Copy(KTestCaseNotSet);
            break;
        }

    __TESTMODEPRINT2("Test set (%d) %S.", iTestCase, &testCaseStr);
    }


void TTestParser::ParseTestEnable(const TDesC8& aTestData)
    {
    __TESTMODEPRINT("Test condition enabled");
    iEnable = ETrue;
    }


void TTestParser::ParseTestSenseError(const TDesC8& aTestData)
    {
    __TESTMODEPRINT("SenseError enabled");

    if (aTestData.Length() < 1)
        iSenseError = ETestSenseErrorNoSense;

    iSenseError = static_cast<TTestSenseError>(aTestData[0]);

    // print test case to debug port
    TBuf<100>senseErrorStr;
    _LIT(KSenseErrorNoSense, "NO SENSE");
    _LIT(KSenseErrorMediaError, "MEDIA ERROR");
    _LIT(KSenseErrorUnitAttention, "UNIT ATTENTION");

    switch (iSenseError)
        {
        case ETestSenseErrorMediaNotPresent:
            senseErrorStr.Copy(KSenseErrorMediaError);
            iSenseError = ETestSenseErrorMediaNotPresent;
            break;

        case ETestSenseErrorUnitAttention:
            senseErrorStr.Copy(KSenseErrorUnitAttention);
            iSenseError = ETestSenseErrorUnitAttention;
            break;

        case ETestSenseErrorNoSense:
        default:
            senseErrorStr.Copy(KSenseErrorNoSense);
            iSenseError = ETestSenseErrorNoSense;
            break;
        }

    __TESTMODEPRINT2("Sense Error set: (%d) %S.", iSenseError, &senseErrorStr);
    }


void TTestParser::ParseTestConfig(const TDesC8& aTestData)
    {
    if (aTestData.Length() >= 1)
        {
        TTestConfig cmd = static_cast<TTestConfig>(aTestData[0]);
        switch (cmd)
            {
            case ETestConfigMediaWpClr:
                __TESTMODEPRINT("WRITE PROTECT is off");
                iWriteProtect = EFalse;
                break;
            case ETestConfigMediaWpSet:
                __TESTMODEPRINT("WRITE PROTECT is on");
                iWriteProtect = ETrue;
                break;
            case ETestConfigMediaRmbClr:
                __TESTMODEPRINT("RMB clr");
                iRemovable = EFalse;
                break;
            case ETestConfigMediaRmbSet:
                __TESTMODEPRINT("RMB set");
                iRemovable = ETrue;
                break;
            default:
                __TESTMODEPRINT("Invalid !");
                break;
            }
        }
    }

#endif
