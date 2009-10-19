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
// SCSI Protocol layer for USB Mass Storage
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef TESTMAN_H
#define TESTMAN_H

#ifdef MSDC_TESTMODE

const TInt KTestCasePreambleLen = 8;
_LIT8(KTxtTestCasePreamble, "5555AAAA*");
_LIT8(KTxtTestCaseDinPreamble, "AAAA5555*");
#define KKeyTestCase        'T'
#define KKeyTestEnable      'E'
#define KKeyTestSenseError  'S'
#define KKeyTestConfig      'C'

class TTestParser
    {
public:
    enum TTestCase
        {
        ETestCaseTagMismatch = 1,
        ETestCaseInvalidSignature = 2,
        // No Data
        ETestCaseNoDataStallCsw = 3,
        ETestCaseNoDataPhaseError = 4,
        // Data OUT
        ETestCaseDoStallCsw = 5,
        ETestCaseDoStallData = 6,
        ETestCaseDoPhaseError = 7,
        ETestCaseDoResidue = 8,
        // Data In
        ETestCaseDiStallCsw = 9,
        ETestCaseDiStallData = 0xA,
        ETestCaseDiPhaseError = 0xB,
        ETestCaseDiResidue = 0xC,
        // End
        ETestCaseNotSet,
        };

    enum TTestConfig
        {
        // Configure Media Write Protect Bit
        ETestConfigMediaWpClr = 1,
        ETestConfigMediaWpSet = 2,
        // Configure Media Removable flag
        ETestConfigMediaRmbClr = 3,
        ETestConfigMediaRmbSet = 4
        };

    enum TTestSenseError
        {
        ETestSenseErrorNoSense,
        ETestSenseErrorMediaNotPresent = 1,
        ETestSenseErrorUnitAttention = 2
        };


    TTestParser();

    TBool DoutSearch(TPtrC8 aBuf);
    TBool DInSearch(TPtrC8 aBuf);

    TTestCase TestCase() const;
    void ClrTestCase();
    void DecTestCounter();
    TInt TestCounter() const {return iTestCounter;}

    TBool Enabled() const {return iEnable;}
    //void Enable() {iEnable = ETrue;}

    TTestSenseError SenseError() const {return iSenseError;}
    TBool WriteProtect() const {return iWriteProtect;}
    TBool Removable() const {return iRemovable;}
    //void SetSenseError() {iSenseError = ETrue;}
    void ClrSenseError() {iSenseError = ETestSenseErrorNoSense;}

    TBool PhaseErrorEnabled() const {return iPhaseErrorEnabled;}
    void SetPhaseError() {iPhaseErrorEnabled = ETrue;}

private:
    void ParseTestCase(const TDesC8& aTestData);
    void ParseTestEnable(const TDesC8& aTestData);
    void ParseTestSenseError(const TDesC8& aTestData);
    void ParseTestConfig(const TDesC8& aTestData);

private:
    TTestCase iTestCase;
    TBool iEnable;
    TTestSenseError iSenseError;
    TBool iPhaseErrorEnabled;
    TInt iTestCounter;
    // Media Config
    TBool iWriteProtect;
    TBool iRemovable;
    };

#endif


#endif // TESTMAN_H
