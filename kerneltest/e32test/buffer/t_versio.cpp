// Copyright (c) 1995-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\t_versio.cpp
// Overview:
// Test the version information class.
// API Information:
// TVersion.
// Details:
// - Test the version name for different versions as expected.
// - Compare the major, minor, build version number of current version and 
// specified version and verify that error message is popped when version
// is not supported.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include "d_version.h"

class RTvTest : public RTest
    {
public:
    RTvTest();
    void TestName();
    void TestQVS();
    void Start(const TDesC &aHeading) {Printf(aHeading);Push();}
    TInt LoadDeviceDriver();
    TInt UnloadDeviceDriver();
private:
    TBool QVS(TInt i,TInt j);
public:
    RVersionTest gLdd;
private:
    TVersion* iTV[KNumTVersions];
    TVersion iDefTV;     // tests default constructor
    };

LOCAL_D const TText* Names[]=
    {
    _S("0.00(0)"),
    _S("0.00(0)"),
    _S("0.00(1)"),
    _S("0.00(999)"),
    _S("0.01(0)"),
    _S("0.01(1)"),
    _S("0.01(999)"),
    _S("0.99(0)"),
    _S("0.99(1)"),
    _S("0.99(999)"),
    _S("1.00(0)"),
    _S("1.00(1)"),
    _S("1.00(999)"),
    _S("1.01(0)"),
    _S("1.01(1)"),
    _S("1.01(999)"),
    _S("1.99(0)"),
    _S("1.99(1)"),
    _S("1.99(999)"),
    _S("99.00(0)"),
    _S("99.00(1)"),
    _S("99.00(999)"),
    _S("99.01(0)"),
    _S("99.01(1)"),
    _S("99.01(999)"),
    _S("99.99(0)"),
    _S("99.99(1)"),
    _S("99.99(999)")
    };

TInt RTvTest::LoadDeviceDriver()
//
// Load device driver for kernel side test
//
    {
    TInt r=KErrNone;
    r=User::LoadLogicalDevice(KVersionTestLddName);
    if(r!=KErrNone)
        {
        return r;
        }
    r=gLdd.Open();
    return r;
    }

TInt RTvTest::UnloadDeviceDriver()
//
// Unload device driver
//
    {
    TInt r=KErrNone;
    gLdd.Close();

    r = User::FreeLogicalDevice(KVersionTestLddName);
    if(r!=KErrNone)
        {
        return r;
        }
    User::After(100000);
    return r;
    }

RTvTest::RTvTest()
//
// Constructor
//
    : RTest(_L("T_VERSIO"))
    {

    iTV[0]=&iDefTV;
    TInt i=1;
    TInt major=0;
    FOREVER
        {
        TInt minor=0;
        FOREVER
            {
            TInt build=0;
            FOREVER
                {
                iTV[i++]=new TVersion(major,minor,build);
                if (build==999)
                    break;
                build=(build==1? 999: 1);
                }
            if (minor==99)
                break;
            minor=(minor==1? 99: 1);
            }
        if (major==99)
            break;
        major=(major==1? 99: 1);
        }
    }

void RTvTest::TestName()
//
// Test the version name
//
    {
 	Next(_L("Testing TVersion::Name()"));
    for (TInt i=0; i<KNumTVersions; i++)
        {
        TPtrC Name=(TPtrC)Names[i];
        if (iTV[i]->Name().Compare(Name))
            Panic(Name);
        }
    }

TBool RTvTest::QVS(TInt aCurrent,TInt aRequested)
//
// An independent calculation of what QueryVersionSupported should return
//
    {
    if (aCurrent)
        aCurrent--;
    if (aRequested)
        aRequested--;
    aCurrent/=3;
    aRequested/=3;
    return(aCurrent>=aRequested);
    }

void RTvTest::TestQVS()
//
// Check QueryVersionSupported()
//
    {
    Next(_L("Testing User::QueryVersionSupported()"));
    for (TInt i=0; i<KNumTVersions; i++)
        {
        for (TInt j=0; j<KNumTVersions; j++)
            {
            if (User::QueryVersionSupported(*iTV[i],*iTV[j])!=QVS(i,j))
                Panic(_L("Query version supported failed"));
            }
        }
    }

GLDEF_C TInt E32Main()
//
// Test TVersion class.
//
    {
    RTvTest test;
	test.Title();
	test.Start(_L("Testing TVersion\n"));
	
	test.LoadDeviceDriver();

	test.Printf(_L("Testing kernel side TVersion::Name()"));
	TInt r = test.gLdd.VersionTestName();
	test(r==KErrNone);

	test.Printf(_L("Testing Kern::QueryVersionSupported()"));
	test.gLdd.VersionTestQVS();
	
	test.UnloadDeviceDriver();
	
    test.TestName();
    test.TestQVS();
    test.Printf(_L("TVersion passed testing\n"));
	test.End();
	return(0);
    }

