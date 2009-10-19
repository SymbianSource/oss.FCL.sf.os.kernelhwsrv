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


#include "cblockdevicetester.h"
#include "cmsdrive.h"
#include "tmsprintdrive.h"
#include "ttestutils.h"
#include "tmslog.h"

extern CMsDrive* msDrive;

RTest test(_L("T_MSSBCERR"));
RFs fsSession;


class CTestSbcErr: public CBase
    {
public:
    static CTestSbcErr* NewL();
    ~CTestSbcErr();
private:
    void ConstructL();
    CTestSbcErr();

public:
    void tTest1();
    void tTest2();
    void tTest3();

private:
    CSbcErrTester* iSbcErrTester;
    };


CTestSbcErr* CTestSbcErr::NewL()
    {
    __MSFNSLOG
	CTestSbcErr* r = new (ELeave) CTestSbcErr();
	CleanupStack::PushL(r);

	r->ConstructL();
	CleanupStack::Pop();
	return r;
    }


void CTestSbcErr::ConstructL()
    {
    __MSFNLOG
    TInt driveNumber = msDrive->DriveNumber();
    iSbcErrTester = CSbcErrTester::NewL(driveNumber);
    }


CTestSbcErr::CTestSbcErr()
    {
    __MSFNLOG
    }



CTestSbcErr::~CTestSbcErr()
    {
    __MSFNLOG
    delete iSbcErrTester;
    }


void CTestSbcErr::tTest1()
    {
    __MSFNLOG
    TInt res;
    test.Start(_L("tTest1\n"));

    // write a file
    res = iSbcErrTester->WriteTestFile();
    test(KErrNone == res);
    // read a file
    res = iSbcErrTester->ReadTestFile();
    // KErrAbort returned due to Media Error
    test(KErrNone == res);

    test.End();
    }


void CTestSbcErr::tTest2()
    {
    __MSFNLOG
    TInt res;
    test.Start(_L("tTest2\n"));

    // Configure client with SENSE ERROR condition
    test.Next(_L("Set SENSE ERROR to MEDIA NOT PRESENT"));
    res = iSbcErrTester->WriteSenseErrorFile(CSbcErrTester::ETestSenseErrorMediaNotPresent);
    test_KErrNone(res);

    test.Next(_L("Read file..."));
    res = iSbcErrTester->ReadTestFile();
    test(KErrNotReady == res);

    // Configure client with SENSE ERROR condition
    test.Next(_L("Set SENSE ERROR to MEDIA NOT PRESENT"));
    res = iSbcErrTester->WriteSenseErrorFile(CSbcErrTester::ETestSenseErrorMediaNotPresent);
    RDebug::Printf("res = %d", res);
    test_KErrNone(res);

    test.Next(_L("Write file..."));
    res = iSbcErrTester->WriteTestFile();
    test(KErrNotReady == res);

    test.End();
    }


void CTestSbcErr::tTest3()
    {
    __MSFNLOG
    TInt res;
    test.Start(_L("tTest3\n"));

    // Configure client with SENSE ERROR condition
    test.Next(_L("Set SENSE ERROR to UNIT ATTENTION"));
    res = iSbcErrTester->WriteSenseErrorFile(CSbcErrTester::ETestSenseErrorUnitAttention);
    test_KErrNone(res);

    test.Next(_L("Read file..."));
    res = iSbcErrTester->ReadTestFile();
    test(KErrNotReady == res);

    // Configure client with SENSE ERROR condition
    test.Next(_L("Set SENSE ERROR to UNIT ATTENTION"));
    res = iSbcErrTester->WriteSenseErrorFile(CSbcErrTester::ETestSenseErrorUnitAttention);
    test_KErrNone(res);

    test.Next(_L("Write file..."));
    res = iSbcErrTester->WriteTestFile();
    test(KErrNotReady == res);

    test.End();
    }




void CallTestsL()
    {
    test.Start(_L("TEST SBC Error Handling"));
    CTestSbcErr* tTest = CTestSbcErr::NewL();

    test.Next(_L("tTest1"));
    tTest->tTest1();
    tTest->tTest2();
    tTest->tTest3();

    delete tTest;
    test.End();
    }
