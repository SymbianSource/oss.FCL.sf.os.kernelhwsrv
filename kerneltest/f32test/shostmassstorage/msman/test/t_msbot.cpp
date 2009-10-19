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
#include <e32property.h>

#include "cblockdevicetester.h"
#include "cmsdrive.h"
#include "tmsprintdrive.h"
#include "ttestutils.h"
#include "tmslog.h"

extern CMsDrive* msDrive;

RTest test(_L("T_MSBOT"));
RFs fsSession;


class RMassStoragePublisher
    {
public:
    enum TPropertyKeys
        {
        KBotResetProperty = 1,
        KStallProperty = 2
        };

    RMassStoragePublisher(TPropertyKeys aKey);
    ~RMassStoragePublisher();

    TInt Count();
    TSecureId Sid() const;

private:
    TPropertyKeys iKey;
    };

TSecureId RMassStoragePublisher::Sid() const
    {
    RProcess process;
    return process.SecureId();
    }

RMassStoragePublisher::RMassStoragePublisher(TPropertyKeys aKey)
:   iKey(aKey)
    {
    _LIT_SECURITY_POLICY_PASS(KHostMassStoragePolicy);
    TInt res = RProperty::Define(iKey,
                                 RProperty::EInt,
                                 KHostMassStoragePolicy,
                                 KHostMassStoragePolicy);

    test(res == KErrNone);
    // reset the property flag
    res = RProperty::Set(Sid(), iKey, 0);
    }


RMassStoragePublisher::~RMassStoragePublisher()
    {
    TInt res = RProperty::Delete(iKey);
    test(res == KErrNone || res == KErrNotFound);
    }


TInt RMassStoragePublisher::Count()
    {
    TInt value;
    TInt res = RProperty::Get(Sid(), iKey, value);
    test_KErrNone(res);
    RDebug::Printf("RMassStoragePublisher[%d] res=%d value=%d", iKey, res, value);
    return value;
    }



/*
class TTestMsBlock
    {
public:
    void tFileAccess();
    void tBlockAccess();
    void tRawAccess();
    void tLastLbaAccessL();
    };
*/

class CTestBot: public CBase
    {
public:
    static CTestBot* NewL();
    ~CTestBot();
private:
    void ConstructL();
    CTestBot();

public:
    void tTagMismatch();
    void tInvalidSignature();
    // No Data
    void tNoDataStallCsw();
    void tNoDataPhaseError();
    // Data OUT
    void tDoStallCsw();
    void tDoStallData();
    void tDoPhaseError();
    void tDoResidue();
    // Data In
    void tDiStallCsw();
    void tDiStallData();
    void tDiPhaseError();
    void tDiResidue();

private:
    void CswResetRecovery(CBotTester::TTestCase aTestCase, const TDesC& aTestName);
    void NoDataResetRecovery(CBotTester::TTestCase aTestCase, const TDesC& aTestName);
    void NoDataClearStall(CBotTester::TTestCase aTestCase, const TDesC& aTestName);

    void DataOutClearStall(CBotTester::TTestCase aTestCase, const TDesC& aTestName);
    void DataInClearStall(CBotTester::TTestCase aTestCase, const TDesC& aTestName);

private:
    CBotTester* iBotTester;
    };




_LIT(KBotTest,          "Bulk Only Transfer Protocol Test");
_LIT(KTagMismatch,      "TagMismatch");
_LIT(KInvalidSignature, "InvalidSignature");
_LIT(KNoDataStallCsw,   "NoDataStallCsw");
_LIT(KNoDataPhaseError, "NoDataPhaseError");
_LIT(KDoStallCsw,       "DoStallCsw");
_LIT(KDoStallData,      "DoStallData");
_LIT(KDoPhaseError,     "DoPhaseError");
_LIT(KDoResidue,        "DoResidue");
_LIT(KDiStallCsw,       "DiStallCsw");
_LIT(KDiStallData,      "DiStallData");
_LIT(KDiPhaseError,     "DiPhaseError");
_LIT(KDiResidue,        "DiResidue");



CTestBot* CTestBot::NewL()
    {
    __MSFNSLOG
	CTestBot* r = new (ELeave) CTestBot();
	CleanupStack::PushL(r);

	r->ConstructL();
	CleanupStack::Pop();
	return r;
    }


void CTestBot::ConstructL()
    {
    __MSFNLOG
    TInt driveNumber = msDrive->DriveNumber();
    iBotTester = CBotTester::NewL(driveNumber);
    }


CTestBot::CTestBot()
    {
    __MSFNLOG
    }



CTestBot::~CTestBot()
    {
    __MSFNLOG
    delete iBotTester;
    }

/**
   USB Mass Storage Class Bulk-Only Transport
   6.5 Host Error Handling
   6.6.1 CBW Not Valid
   5.3.3.1 Phase Error

   @param aTestCase
   @param testName
 */
void CTestBot::CswResetRecovery(CBotTester::TTestCase aTestCase, const TDesC& aTestName)
    {
    __MSFNLOG
    TInt res;
    test.Start(aTestName);

    RMassStoragePublisher msPublisher(RMassStoragePublisher::KBotResetProperty);

    // Set test case in client
    res = iBotTester->SetTest(aTestCase);
    test_KErrNone(res);
    // Enable test condition. File transfer is successful as the file server
    // retries in the event of a transfer error.
    test.Printf(_L("Writing file\n"));//
    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    if (aTestCase == CBotTester::ETestCaseDiPhaseError)
        {
        test.Printf(_L("Reading file\n"));
        res = iBotTester->ReadEnableFile();
        }

    // Check that Host did a BOT Reset Recovery
    TInt botReset = msPublisher.Count();
    test(botReset==1);
    test.Printf(_L("Host performed Reset Recovery.\n"));
    test.End();
    }


void CTestBot::NoDataResetRecovery(CBotTester::TTestCase aTestCase, const TDesC& aTestName)
    {
    __MSFNLOG
    TInt res;
    test.Start(aTestName);

    RMassStoragePublisher msPublisher(RMassStoragePublisher::KBotResetProperty);

    // Set test case in client
    res = iBotTester->SetTest(aTestCase);
    test_KErrNone(res);

    // Enable test condition.
    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    // Wait for the host to send TEST UNIT READY. The client is configured to
    // respond with a Phase Error. Host should perform RESET RECOVERY

    // Check that Host did a BOT Reset Recovery
    User::After(1000*1000*30);
    TInt count = msPublisher.Count();
    test(count == 1);
    test.End();
    }


void CTestBot::NoDataClearStall(CBotTester::TTestCase aTestCase, const TDesC& aTestName)
    {
    __MSFNLOG
    TInt res;
    test.Start(aTestName);

    RMassStoragePublisher msPublisher(RMassStoragePublisher::KStallProperty);

    // Set test case in client
    res = iBotTester->SetTest(aTestCase);
    test_KErrNone(res);

    // Enable test condition.
    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    // Wait for the host to send TEST UNIT READY. The client is configured to
    // stall this request. Host should clear stall condition and attempt to
    // receive CSW again.

    /*
    // Cause host to send TEST UNIT READY with a client SENSE ERROR condition
    res = iBotTester->WriteSenseErrorFile();
    test_KErrNone(res);

    // read a file
    res = iBotTester->ReadTestFile();
    // KErrAbort returned due to Media Error
    test(KErrNotReady == res);
    */

    // Check that host cleared the Stall condition
    User::After(1000*1000*30);
    TInt i = msPublisher.Count();
    test(i == 1);
    test.End();
    }


void CTestBot::DataOutClearStall(CBotTester::TTestCase aTestCase, const TDesC& aTestName)
    {
    __MSFNLOG
    TInt res;
    test.Start(aTestName);

    RMassStoragePublisher msPublisher(RMassStoragePublisher::KStallProperty);

    // Set test case in client
    res = iBotTester->SetTest(aTestCase);
    test_KErrNone(res);
    // Enable test condition. File transfer is successful as the file server
    // retries in the event of a transfer error.
    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    if (aTestCase == CBotTester::ETestCaseDoStallData)
        {
        TLba lba = 0x5;
        TLba blocks = 0x4;
        test.Printf(_L("Writing data LBA=0x%x Blocks=0x%x\n"), lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        res = iBotTester->UpdateBlock(lba, blocks);
        }

    // Check that Host cleared the Stall condition
    TInt i = msPublisher.Count();
    test(i == 1);
    test.End();
    }


void CTestBot::DataInClearStall(CBotTester::TTestCase aTestCase, const TDesC& aTestName)
    {
    __MSFNLOG
    TInt res;
    test.Start(aTestName);

    RMassStoragePublisher msPublisher(RMassStoragePublisher::KStallProperty);

    // Set test case in client
    res = iBotTester->SetTest(aTestCase);
    test_KErrNone(res);
    // Enable test condition. File transfer is successful as the file server
    // retries in the event of a transfer error.
    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    test.Next(_L("Read file and check Data-In stall\n"));
    if (aTestCase == CBotTester::ETestCaseDiStallCsw)
        {
        res = iBotTester->ReadEnableFile();
        test.Printf(_L("File read returned %d.\n"), res);
        test_KErrNone(res);
        }
    else if (aTestCase == CBotTester::ETestCaseDiStallData)
        {
        res = iBotTester->ReadEnableFile();
        test.Printf(_L("File read returned %d.\n"), res);
        test_KErrNone(res);
        }

    // Check that Test client actually stalled. Note that the next CSW is
    // invalid and the host will also perform Reset Recovery
    TInt botStall = msPublisher.Count();
    test(botStall==1);

    test.Next(_L("Check next read is successful\n"));
    res = iBotTester->ReadEnableFile();
    test.Printf(_L("File read returned %d.\n"), res);
    test_KErrNone(res);

    test.Printf(_L("Host cleared stall on Bulk-In pipe.\n"));
    test.End();
    }


/**
Tag Mismatch:
    1. CSW tag does not match CBW tag
    2. CSW is not valid
    3. Host shall perform a Reset Recovery
*/
void CTestBot::tTagMismatch()
    {
    __MSFNLOG
    CswResetRecovery(CBotTester::ETestCaseTagMismatch, KTagMismatch);
    }


/**
Invalid Signature:
    1. CSW Signature is not equal to 53425355h.
    2. CSW is not valid.
    3. Host shall perform a Reset Recovery.
*/
void CTestBot::tInvalidSignature()
    {
    __MSFNLOG
    CswResetRecovery(CBotTester::ETestCaseInvalidSignature, KInvalidSignature);
    }


/**
Hn CSW stall condition
    1. Host expects no data transfer and CSW is stalled.
    2. Host shall clear the Bulk-In pipe.
    3. Host shall attempt to receive the CSW again.
*/
void CTestBot::tNoDataStallCsw()
    {
    __MSFNLOG
    NoDataClearStall(CBotTester::ETestCaseNoDataStallCsw, KNoDataStallCsw);
    }

/**
Hn CSW Phase Error status
    1. Host expect no data transfer and CSW status = 02h (Phase Error.
    2. Host shall peroform a reset recovery.
*/
void CTestBot::tNoDataPhaseError()
    {
    __MSFNLOG
    NoDataResetRecovery(CBotTester::ETestCaseNoDataPhaseError, KNoDataPhaseError);
    }


void CTestBot::tDoStallCsw()
    {
    __MSFNLOG
    DataOutClearStall(CBotTester::ETestCaseDoStallCsw, KDoStallCsw);
    }


void CTestBot::tDoStallData()
    {
    __MSFNLOG
    DataOutClearStall(CBotTester::ETestCaseDoStallData, KDoStallData);
    }


void CTestBot::tDoPhaseError()
    {
    __MSFNLOG
    CswResetRecovery(CBotTester::ETestCaseDoPhaseError, KDoPhaseError);
    }


void CTestBot::tDoResidue()
    {
    __MSFNLOG
    TInt res;
    test.Start(KDoResidue);
    res = iBotTester->SetTest(CBotTester::ETestCaseDoResidue);
    test_KErrNone(res);
    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    test.End();
    }


void CTestBot::tDiStallCsw()
    {
    __MSFNLOG
    DataInClearStall(CBotTester::ETestCaseDiStallCsw, KDiStallCsw);
    }


void CTestBot::tDiStallData()
    {
    __MSFNLOG
    DataInClearStall(CBotTester::ETestCaseDiStallData, KDiStallData);
    }


void CTestBot::tDiPhaseError()
    {
    __MSFNLOG
    CswResetRecovery(CBotTester::ETestCaseDiPhaseError, KDiPhaseError);
    }


void CTestBot::tDiResidue()
    {
    __MSFNLOG
    TInt res;
    test.Start(KDiResidue);

    // First write test file to drive
    res = iBotTester->InitReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->SetTest(CBotTester::ETestCaseDiResidue);
    test_KErrNone(res);

    res = iBotTester->WriteEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    res = iBotTester->ReadEnableFile();
    test_KErrNone(res);

    test.End();
    }


void CallTestsL()
    {
    test.Start(KBotTest);
    CTestBot* tBot = CTestBot::NewL();

    test.Next(KTagMismatch);
    tBot->tTagMismatch();

    test.Next(KInvalidSignature);
    tBot->tInvalidSignature();

    test.Next(KNoDataStallCsw);
    tBot->tNoDataStallCsw();

    test.Next(KNoDataPhaseError);
    tBot->tNoDataPhaseError();

    test.Next(KDoStallCsw);
    tBot->tDoStallCsw();

//    test.Next(KDoStallData);        // not working in test client
//    tBot->tDoStallData();

    test.Next(KDoPhaseError);
    tBot->tDoPhaseError();

    test.Next(KDiStallCsw);
    tBot->tDiStallCsw();

    test.Next(KDiStallData);
    tBot->tDiStallData();

    test.Next(KDiPhaseError);
    tBot->tDiPhaseError();

    test.Next(KDoResidue);
    tBot->tDoResidue();

    test.Next(KDiResidue);
    tBot->tDiResidue();

    delete tBot;
    test.End();
    }
