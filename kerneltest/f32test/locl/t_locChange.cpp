// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32test.h>
#include <f32file.h>
#include <e32svr.h>

_LIT(KGerLocale, "T_LOCLGE.DLL");
_LIT(KEngLocale, "T_LOCLUS.DLL");

#ifndef __WINS__  
_LIT(KEngLocaleRAM, "T_LOCLUS_RAM.DLL"); //this should be RAM-loaded library.. 
#else 
_LIT(KEngLocaleRAM, "T_LOCLUS.DLL"); 
#endif
    
// 
class RTestSafeLocale : public RTest
    {
    public:
    RTestSafeLocale(const TDesC &aTitle):  RTest(aTitle),  iFailHdnFunc(NULL) {}
    RTestSafeLocale(const TDesC &aTitle, void(*func)(RTest &aTest)) : RTest(aTitle), iFailHdnFunc(func) {}
    
    //new wersion of operator, which calls handler if check failed
    void operator()(TInt aResult)
        {
        if (!aResult && iFailHdnFunc) iFailHdnFunc(*this);
        RTest::operator ()(aResult);
        }
    
    //new version of End, which calls handler before exit..
    IMPORT_C void End() 
        { 
        if (iFailHdnFunc) iFailHdnFunc(*this);
        RTest::End();
        }
    
    //pointer to handler..
    void (*iFailHdnFunc)(RTest &aTest);
    };
    
// cleanup handler, which restores default locale on test end or failure..
void TestCleanup(RTest &aTest)
    {
    aTest.Printf(_L("\nTest cleanup: changing locale to default..\n"));
    UserSvr::ChangeLocale(_L(""));
    aTest.Printf(_L("Default language: %d\n"), User::Language());
    }

// global gTest object..
RTestSafeLocale gTest(_L("T_LOCCHANGE"), &TestCleanup);

// try to load locale dll prior to changing it..
TInt LoadLocaleCrash(const TDesC& aName)
    {
    //First - load the library.. 
    RLibrary lib;
    TInt err = lib.Load(aName);
    if (err)
        {
        gTest.Printf(_L("\nRLibrary::Load() failed, err %d"), err);
        return err;
        }
    // try to change locale.. (it should ignore the previously loaded library.. 
    // and load locale library again in the global area.
    err = UserSvr::ChangeLocale(aName);
    if (err)
        {
        gTest.Printf(_L("\nUserSvr::ChangeLocale() failed, err %d"), err);
        return err;
        }

    lib.Close();
    return KErrNone;
    }

// change locale normally..
TInt LoadLocale(const TDesC& aName)
    {
    TInt r = UserSvr::ChangeLocale(aName);
    if (r != KErrNone)
        return r;
    return KErrNone;
    }

// main..
TInt E32Main()
    {
    gTest.Start(_L("Test Locale Change\n"));

    TInt r;
    RChangeNotifier notifier;
    TRequestStatus status;
    gTest(notifier.Create() == KErrNone);
    gTest(notifier.Logon(status) == KErrNone);
    User::WaitForRequest(status);

    // Monitor locale change event
    gTest(notifier.Logon(status) == KErrNone);
    
    r = LoadLocale(KGerLocale);
    gTest(r == KErrNone);
    User::WaitForRequest(status);
    gTest(status.Int() & EChangesLocale);
    gTest.Printf(_L("New Language: %d\n"), User::Language());
    gTest(notifier.Logon(status) == KErrNone);

    r = LoadLocale(KEngLocale);
    gTest(r == KErrNone);
    User::WaitForRequest(status);
    gTest(status.Int() & EChangesLocale);
    gTest.Printf(_L("New Language: %d\n"), User::Language());
    gTest(notifier.Logon(status) == KErrNone);

    r = LoadLocaleCrash(KEngLocaleRAM);
    gTest(r == KErrNone);
    User::WaitForRequest(status);
    gTest(status.Int() & EChangesLocale);
    gTest.Printf(_L("New Language: %d\n"), User::Language());
    
    notifier.Close();
    gTest.End();
    return 0;
    }
