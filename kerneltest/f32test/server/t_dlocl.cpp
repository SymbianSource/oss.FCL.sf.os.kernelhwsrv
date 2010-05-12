// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_dlocl.cpp
// Tests UserSvr::ChangeLocale() function
// 
//

#define __E32TEST_EXTENSION__

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <f32file.h>
#include <e32svr.h>
#include <collate.h>

#include "t_server.h"

LOCAL_D TFindLibrary fLib;

typedef TInt (*TLibEntry)(TInt);
typedef TInt (*TLibEntry2)();

const TBool KEY_DOWN=EFalse;
const TBool EXPECT_KEY_PRESS=ETrue;

RTest test(_L("T_DLOCL"));

_LIT(ELOCL_DEFAULT, "");
_LIT(ELOCLGE, "T_LOCLGE");
_LIT(ELOCLUS, "T_LOCLUS");
_LIT(ELOCLUS1, "T_LOCLUS1");
_LIT(ELOCLUS2, "T_LOCLUS2");
_LIT(EKDATA, "EKDATA");
_LIT(DUMMYDLL, "EDISP");
_LIT(KColonColon, "::");
_LIT(KDLLExtension, ".DLL");

GLDEF_D CKeyTranslator *KeyTranslator=CKeyTranslator::New();
LOCAL_D CCaptureKeys *CaptureKeys;

void testConv(const TDesC& aDes,TBool aKeyup,TBool aRet,TUint aScanCode,TUint aKeyCode,TInt aModifiers)
    {

    TKeyData keyData;
    TBool ret=KeyTranslator->TranslateKey(aScanCode, aKeyup,*CaptureKeys,keyData);
    test.Next(aDes);
    test(ret==aRet);
    test((keyData.iKeyCode==aKeyCode));
    test((keyData.iModifiers==aModifiers));
    }

void testChangeKeyData()
    {
    TName pn(RProcess().Name());
    pn+=KColonColon;
    TFullName n;
    CaptureKeys=new CCaptureKeys();
    CaptureKeys->Construct();
    test.Printf(_L("Test default key data\n"));
    testConv(_L("\nFirst Special key down"),KEY_DOWN,EXPECT_KEY_PRESS,ESpecialKeyBase,ESpecialKeyBase,0);

    RLibrary testLib;
    TInt res=testLib.Load(EKDATA);
    test_KErrNone(res);
    THandleInfo handleInfo;
    testLib.HandleInfo(&handleInfo);
    test(handleInfo.iNumOpenInThread==2);
    testLib.Close();

    test.Printf(_L("Change to unknown dll \n"));                // Test with non keydata type dll
    res=KeyTranslator->ChangeKeyData(DUMMYDLL);
    test_Value(res, res == KErrArgument);
    
    res=testLib.Load(EKDATA);
    test_KErrNone(res);
    testLib.HandleInfo(&handleInfo);
    test(handleInfo.iNumOpenInThread==2);
    testLib.Close();

    res=testLib.Load(DUMMYDLL);
    test_KErrNone(res);
    testLib.HandleInfo(&handleInfo);
    test(handleInfo.iNumOpenInThread==1);
    testLib.Close();

    fLib.Find(_L("*"));
    while (fLib.Next(n)==KErrNone)
        {
        TName findname=pn;
        findname+=DUMMYDLL;
        test.Printf(_L("  %S\n"),&n);        
        test(n.FindF(findname) == KErrNotFound);
        }
    //
    test.Printf(_L("Change to EKDATA.dll\n"));
    res=KeyTranslator->ChangeKeyData(EKDATA);
    test_KErrNone(res);
    
    res=testLib.Load(EKDATA);
    test_KErrNone(res);
    testLib.HandleInfo(&handleInfo);
    test(handleInfo.iNumOpenInThread==2);
    testLib.Close();
    res=testLib.Load(DUMMYDLL);
    test_KErrNone(res);
    testLib.HandleInfo(&handleInfo);
    test(handleInfo.iNumOpenInThread==1);
    testLib.Close();

    fLib.Find(_L("*"));
    while (fLib.Next(n)==KErrNone)
        {
        TName findname=pn;
        findname+=EKDATA;
        test.Printf(_L("  %S\n"),&n);        
        if(n.FindF(findname) != KErrNotFound)
            break;
        }
    //The test below fails if we use Secure APIs
    //test(n.FindF(EKDATA) == KErrNotSupported);
    testConv(_L("\nFirst Special key down"),KEY_DOWN,EXPECT_KEY_PRESS,ESpecialKeyBase,ESpecialKeyBase,0);

    test.Printf(_L("Change back to Default KeyData\n"));
    res=KeyTranslator->ChangeKeyData(_L(""));
    test_KErrNone(res);
    
    res=testLib.Load(EKDATA);
    test_KErrNone(res);
    testLib.HandleInfo(&handleInfo);
    test(handleInfo.iNumOpenInThread==2);
    testLib.Close();

    res=testLib.Load(DUMMYDLL);
    test_KErrNone(res);
    testLib.HandleInfo(&handleInfo);
    test(handleInfo.iNumOpenInThread==1);
    testLib.Close();

    fLib.Find(_L("*"));
    while (fLib.Next(n)==KErrNone)
        {
        TName findname=pn;
        findname+=DUMMYDLL;
        test.Printf(_L("  %S\n"),&n);        
        test(n.FindF(findname) == KErrNotFound);
        }
    testConv(_L("\nFirst Special key down"),KEY_DOWN,EXPECT_KEY_PRESS,ESpecialKeyBase,ESpecialKeyBase,0);
    delete CaptureKeys;
    }

void testUS(const TLocale& aLocale)
    {
//#ifdef __WINS__
    test(aLocale.CountryCode()==1);
    test(aLocale.DateFormat()==EDateAmerican);
    test(aLocale.TimeFormat()==ETime12);
    test(aLocale.CurrencySymbolPosition()==ELocaleBefore);
    test(aLocale.CurrencySpaceBetween()==FALSE);
    test(aLocale.CurrencyDecimalPlaces()==2);
    test(aLocale.CurrencyNegativeInBrackets()==EFalse);
    test(aLocale.CurrencyTriadsAllowed()==TRUE);
    test(aLocale.ThousandsSeparator()==',');
    test(aLocale.DecimalSeparator()=='.');
    test(aLocale.DateSeparator(0)==0);
    test(aLocale.DateSeparator(1)=='/');
    test(aLocale.DateSeparator(2)=='/');
    test(aLocale.DateSeparator(3)==0);
    test(aLocale.TimeSeparator(0)==0);
    test(aLocale.TimeSeparator(1)==':');
    test(aLocale.TimeSeparator(2)==':');
    test(aLocale.TimeSeparator(3)==0);
    test(aLocale.AmPmSymbolPosition()==TRUE);
    test(aLocale.AmPmSpaceBetween()==TRUE);
    test(aLocale.HomeDaylightSavingZone()==EDstNorthern);
    test(aLocale.WorkDays()==0x1f);
    test(aLocale.StartOfWeek()==ESunday);
    test(aLocale.ClockFormat()==EClockAnalog);
    test(aLocale.UnitsGeneral()==EUnitsImperial);
    test(aLocale.UnitsDistanceShort()==EUnitsImperial);
    test(aLocale.UnitsDistanceLong()==EUnitsImperial);
//#endif
    }


void testUK(const TLocale& aLocale)
    {
//#ifdef __WINS__
    test(aLocale.CountryCode()==44);
    test(aLocale.DateFormat()==EDateEuropean);
    test(aLocale.TimeFormat()==ETime12);
    test(aLocale.CurrencySymbolPosition()==ELocaleBefore);
    test(aLocale.CurrencySpaceBetween()==FALSE);
    test(aLocale.CurrencyDecimalPlaces()==2);
    test(aLocale.CurrencyNegativeInBrackets()==EFalse);
    test(aLocale.CurrencyTriadsAllowed()==TRUE);
    test(aLocale.ThousandsSeparator()==',');
    test(aLocale.DecimalSeparator()=='.');
    test(aLocale.DateSeparator(0)==0);
    test(aLocale.DateSeparator(1)=='/');
    test(aLocale.DateSeparator(2)=='/');
    test(aLocale.DateSeparator(3)==0);
    test(aLocale.TimeSeparator(0)==0);
    test(aLocale.TimeSeparator(1)==':');
    test(aLocale.TimeSeparator(2)==':');
    test(aLocale.TimeSeparator(3)==0);
    test(aLocale.AmPmSymbolPosition()==TRUE);
    test(aLocale.AmPmSpaceBetween()==TRUE);
    test(aLocale.HomeDaylightSavingZone()==EDstEuropean);
    test(aLocale.WorkDays()==0x1f);
    test(aLocale.StartOfWeek()==EMonday);
    test(aLocale.ClockFormat()==EClockAnalog);
    test(aLocale.UnitsGeneral()==EUnitsImperial);
    test(aLocale.UnitsDistanceShort()==EUnitsImperial);
    test(aLocale.UnitsDistanceLong()==EUnitsImperial);
//#endif
    }

void testGE(const TLocale& aLocale)
    {
//#ifdef __WINS__
    test(aLocale.CountryCode()==49);
    test(aLocale.DateFormat()==EDateEuropean);
    test(aLocale.TimeFormat()==ETime24);
    test(aLocale.CurrencySymbolPosition()==ELocaleAfter);
    test(aLocale.CurrencySpaceBetween()==TRUE);
    test(aLocale.CurrencyDecimalPlaces()==2);
    test(aLocale.CurrencyNegativeInBrackets()==TRUE);
    test(aLocale.CurrencyTriadsAllowed()==TRUE);
    test(aLocale.ThousandsSeparator()=='.');
    test(aLocale.DecimalSeparator()==',');
    test(aLocale.DateSeparator(0)==0);
    test(aLocale.DateSeparator(1)=='.');
    test(aLocale.DateSeparator(2)=='.');
    test(aLocale.DateSeparator(3)==0);

    test(aLocale.TimeSeparator(0)==0);
    test(aLocale.TimeSeparator(1)==':');
    test(aLocale.TimeSeparator(2)==':');
    test(aLocale.TimeSeparator(3)==0);
    test(aLocale.AmPmSymbolPosition()==TRUE);
    test(aLocale.AmPmSpaceBetween()==TRUE);
    test(aLocale.HomeDaylightSavingZone()==EDstEuropean);
    test(aLocale.WorkDays()==0x1f);
    test(aLocale.StartOfWeek()==EMonday);
    test(aLocale.ClockFormat()==EClockDigital);
    test(aLocale.UnitsGeneral()==EUnitsMetric);
    test(aLocale.UnitsDistanceShort()==EUnitsMetric);
    test(aLocale.UnitsDistanceLong()==EUnitsMetric);
//#endif
    }



/**
    Subscribe to a system event.

    @param  aNotifier   notifier
    @param  aStatus     request status that is used with the notifier
    @param  aEventMask  Specifies the event. See TChanges
*/
static void SubscribeToSystemChangeNotification(RChangeNotifier& aNotifier, TRequestStatus& aStatus, TUint32 aEventMask)
{
    
    const TInt KMaxAttempts = 100;
    TInt i;
    for(i=0; i<KMaxAttempts; ++i)
    {
        test.Printf(_L("SubscribeToSystemChangeNotification(0x%x), attempt:%d\n"), aEventMask, i);   

        TInt nRes = aNotifier.Logon(aStatus);
        test_KErrNone(nRes);

        if(aStatus.Int() == KRequestPending)
            break;

        //-- some other system-wide event can just happen; re-subscribe
        test( !(aStatus.Int() & aEventMask));
    }

    test(i<KMaxAttempts);
}

/**
    Wait for the event(s) specified in aEventMask to happen

    @param  aNotifier   notifier
    @param  aStatus     request status that is used with the notifier
    @param  aEventMask  Specifies the event. See TChanges
*/
static void WaitForSystemChange(RChangeNotifier& aNotifier, TRequestStatus& aStatus, TUint32 aEventMask)
{
    //-- it would be nice to have here a timeout processing in order not to wait forever.. but later.
    for(;;)
    {
        User::WaitForRequest(aStatus);
        if(aStatus.Int() & aEventMask)
            return; 

        //-- some other system-wide unexpected event happened, we need to resubscribe
        test.Printf(_L("WaitForSystemChange(0x%x), happened:0x%x, resubscribing...\n"), aEventMask, aStatus.Int());   
        SubscribeToSystemChangeNotification(aNotifier, aStatus, aEventMask);
    }

}
    

void testChangeLocale()
    {
    TLocale locale;
    
    RChangeNotifier notifier;
    TInt res=notifier.Create();
    test_KErrNone(res);
    TRequestStatus stat;
    
    res=notifier.Logon(stat);
    test_KErrNone(res);
	// initial value of stat already tested by t_chnot

    SubscribeToSystemChangeNotification(notifier, stat, EChangesLocale);

        test.Printf(_L("Change to US Locale\n"));   
        res=UserSvr::ChangeLocale(ELOCLUS);
        test.Printf(_L("res=%d\n"),res);
        test_KErrNone(res);

    WaitForSystemChange(notifier, stat, EChangesLocale);
    test(stat.Int() & EChangesLocale);
    
    //-------------------
    SubscribeToSystemChangeNotification(notifier, stat, EChangesLocale);
        
        locale.Refresh();
        // let's not test localisation data details now that the internationalisation
        // team rather than us release the localisation dlls (changed 9/6/98)
        testUS(locale);

        test.Printf(_L("Change to GE Locale\n"));
        res=UserSvr::ChangeLocale(ELOCLGE);
        test.Printf(_L("res=%d\n"),res);
        test_KErrNone(res);

    WaitForSystemChange(notifier, stat, EChangesLocale);
    test(stat.Int() & EChangesLocale);
    
    //-------------------
    
    SubscribeToSystemChangeNotification(notifier, stat, EChangesLocale);
        
        locale.Refresh();
        // let's not test localisation data details now that the internationalisation
        // team rather than us release the localisation dlls (changed 9/6/98)
        testGE(locale);

        test.Printf(_L("Load non ELOCL type DLL\n"));    
        res=UserSvr::ChangeLocale(DUMMYDLL);
        test.Printf(_L("res=%d\n"),res);
        test_Value(res, res == KErrNotSupported);
    
    //-- ensure that there wasn't locale change
    const TInt KMaxAttempts = 100;
	TInt i;
    for(i=0; i<KMaxAttempts; ++i)
    {
        if(stat ==KRequestPending)
            break;
        
        //-- check that if something system-wide happened, it wasn't a locale change
        test(!(stat.Int() & EChangesLocale));
        SubscribeToSystemChangeNotification(notifier, stat, EChangesLocale);
    }
    test(i<KMaxAttempts);


        locale.Refresh();
        // let's not test localisation data details now that the internationalisation
        // team rather than us release the localisation dlls (changed 9/6/98)
        testGE(locale);

        //This test using US1 locale is for identifying different collation table
        //used for matching and comparing. To specify a table to be used in the
        //matching we just need to add the TCollationTable::EMatchingTable flag
        //inside the TCollationMethods::iFlags
        test.Printf(_L("Change to US1 Locale\n"));  
        res=UserSvr::ChangeLocale(ELOCLUS1);
        test.Printf(_L("res=%d\n"),res);
        test_KErrNone(res);
    
    WaitForSystemChange(notifier, stat, EChangesLocale);
    test(stat.Int() & EChangesLocale);

    
    //-------------------
    SubscribeToSystemChangeNotification(notifier, stat, EChangesLocale);

        locale.Refresh();
    
    //Testing the different collation table used for Matching and Comparing
    //after loading this new locale using T_LOCLUS1.DLL.
    //test.Printf(_L("Test for differnt collation table for Matching and Comparing\n"));
    //TCollationMethod m=*Mem::GetDefaultMatchingTable();
    //test(m.iFlags & TCollationMethod::EMatchingTable);
    
    //The collation table t_locl.cpp can be found in \locl\t_locl.cpp
    //Expected collation override for MatchC A==B and a==b
    //Expected collation override for CompareC A>B and a>b
    //Collation tables cannot be changed at runtime
/*  TBuf16<1> a_cap(_L("A"));
    TBuf16<1> a_small(_L("a"));
    
    //Test using the locale independent matching and comparing
    test(a_cap.Match(_L("B"))!=0);
    test(a_cap.Compare(_L("B"))<0);
    test(a_small.Match(_L("b"))!=0);
    test(a_small.Compare(_L("b"))<0);
    //Test that the default collation table(1st table) is selected here
    test(a_cap.CompareC(_L("B"))!=0);
    test(a_small.CompareC(_L("b"))!=0);
    test(a_cap.CompareC(_L("B"))>0);
    test(a_small.CompareC(_L("b"))>0);
    //Test that the Matching collation table(3rd table) is selected here
    test(a_cap.MatchC(_L("B"))==0);
    test(a_small.MatchC(_L("b"))==0);
*/
        test.Printf(_L("Back to default UK Locale\n"));
        res=UserSvr::ChangeLocale(ELOCL_DEFAULT);
        test.Printf(_L("res=%d\n"),res);
        test_KErrNone(res);
    
    WaitForSystemChange(notifier, stat, EChangesLocale);
    test(stat.Int() & EChangesLocale);
    
    //-------------------

    SubscribeToSystemChangeNotification(notifier, stat, EChangesLocale);

        locale.Refresh();
        testUK(locale); 
    
        //Test for locale which does not have a matching collation table
        TCollationMethod m1=*Mem::GetDefaultMatchingTable();
        test((m1.iFlags & TCollationMethod::EMatchingTable)==0);

        //************************************************
        test.Printf(_L("Change to US Locale with file open on drive\n"));   
    
        _LIT(KTestFile, "TEST.TXT");
        RFile file;
        res = file.Replace(TheFs, KTestFile, 0);
        test_KErrNone(res);


        res=UserSvr::ChangeLocale(ELOCLUS);
        test.Printf(_L("res=%d\n"),res);
        test_KErrNone(res);


    WaitForSystemChange(notifier, stat, EChangesLocale);
    test(stat.Int() & EChangesLocale);

    
    //-------------------
    SubscribeToSystemChangeNotification(notifier, stat, EChangesLocale);


        _LIT8(KTestData, "Arsenal");
        res = file.Write(KTestData);
        test_KErrNone(res);
        file.Close();

        res = file.Open(TheFs, KTestFile, 0);
        test_KErrNone(res);
        file.Close();
        res = TheFs.Delete(KTestFile);
        test_KErrNone(res);

    //************************************************

    //-- close the notifier
    notifier.Close();
    User::WaitForRequest(stat);
    
    //-- n.b. it's actually a bad idea to expect _exact_ event completion from the RChangeNotifier, because
    //-- this is a system-wide events observer and any of the TChanges events can happen, not only EChangesLocale.
    //-- so this test is a bit flawed.

    //test(stat==KErrGeneral);
    }

const TText * const DateSuffixTable[KMaxSuffixes] =
    {
    _S("st"),_S("nd"),_S("rd"),_S("th"),_S("th"),
    _S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
    _S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
    _S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
    _S("st"),_S("nd"),_S("rd"),_S("th"),_S("th"),
    _S("th"),_S("th"),_S("th"),_S("th"),_S("th"),
    _S("st")
    };

const TText * const DayTable[KMaxDays] =
    {
    _S("Monday"),
    _S("Tuesday"),
    _S("Wednesday"),
    _S("Thursday"),
    _S("Friday"),
    _S("Saturday"),
    _S("Sunday")
    };

const TText * const DayAbbTable[KMaxDays] =
    {
    _S("Mon"),
    _S("Tue"),
    _S("Wed"),
    _S("Thu"),
    _S("Fri"),
    _S("Sat"),
    _S("Sun")
    };

const TText * const MonthTable[KMaxMonths] =
    {
    _S("January"),
    _S("February"),
    _S("March"),
    _S("April"),
    _S("May"),
    _S("June"),
    _S("July"),
    _S("August"),
    _S("September"),
    _S("October"),
    _S("November"),
    _S("December")
    };

const TText * const MonthAbbTable[KMaxMonths] =
    {
    _S("Jan"),
    _S("Feb"),
    _S("Mar"),
    _S("Apr"),
    _S("May"),
    _S("Jun"),
    _S("Jul"),
    _S("Aug"),
    _S("Sep"),
    _S("Oct"),
    _S("Nov"),
    _S("Dec")
    };

const TText * const AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};

void testExtendedUS(TUint aAspect, TExtendedLocale& aLocale)
    {
    TLocale* loc = aLocale.GetLocale();
    testUS(*loc);
    
    if(aAspect & ELocaleLanguageSettings)
        {
        TLanguage tl = User::Language();
        test(tl == ELangAmerican);
        
        TDateSuffix datesuffix;
        TInt i;
        for(i=0;i<KMaxSuffixes;++i)
            {
            datesuffix.Set(i);
            test(datesuffix.Compare(TPtrC(DateSuffixTable[i])) == 0);
            }

        TDayName dayname;
        for(i=0;i<KMaxDays;++i)
            {
            dayname.Set((TDay)i);
            test(dayname.Compare(TPtrC(DayTable[i])) == 0);
            }

        TDayNameAbb daynameabb;
        for(i=0;i<KMaxDays;++i)
            {
            daynameabb.Set((TDay)i);
            test(daynameabb.Compare(TPtrC(DayAbbTable[i])) == 0);
            }

        TMonthName monthname;
        for(i=0;i<KMaxMonths;++i)
            {
            monthname.Set((TMonth)i);
            test(monthname.Compare(TPtrC(MonthTable[i])) == 0);
            }

        TMonthNameAbb monthnameabb;
        for(i=0;i<KMaxMonths;++i)
            {
            monthnameabb.Set((TMonth)i);
            test(monthnameabb.Compare(TPtrC(MonthAbbTable[i])) == 0);
            }

        TAmPmName ampmname;
        for(i=0;i<KMaxAmPms;++i)
            {
            ampmname.Set((TAmPm)i);
            test(ampmname.Compare(TPtrC(AmPmTable[i])) == 0);
            }
        }

    if(aAspect & ELocaleLocaleSettings)
        {
        TCurrencySymbol symbol;
        symbol.Set();
        test(symbol.Compare(TPtrC(_S("$"))) == 0);
        }

    if(aAspect & ELocaleTimeDateSettings)
        {
        TShortDateFormatSpec shortdate;
        shortdate.Set();
        test(shortdate.Compare(TPtrC(_S("%F%*M/%*D/%Y"))) == 0);

        TLongDateFormatSpec longdate;
        longdate.Set();
        test(longdate.Compare(TPtrC(_S("%F%*D%X %N %Y"))) == 0);

        TTimeFormatSpec spec;
        spec.Set();
        test(spec.Compare(TPtrC(_S("%F%*I:%T:%S %*A"))) == 0);
        }

    }

    
void testExtendedLocale()
    {
    TExtendedLocale locale;
    locale.LoadLocale(ELOCLUS);
    TInt r = locale.SaveSystemSettings();
    test_KErrNone(r);
    testExtendedUS(ELocaleLanguageSettings | ELocaleCollateSetting | ELocaleLocaleSettings | ELocaleTimeDateSettings, locale);

    r = locale.SetCurrencySymbol(TPtrC(_S("Leu")));
    test_KErrNone(r);
    TCurrencySymbol symbol;
    symbol.Set();
    test(symbol.Compare(TPtrC(_S("Leu"))) == 0);

    User::SetCurrencySymbol(TPtrC(_S("Le")));
    symbol.Set();
    test(symbol.Compare(TPtrC(_S("Le"))) == 0);

    TFileName dllName;
    TFileName eloclus;
    eloclus.Copy(ELOCLUS);
    eloclus.Append(TPtrC(KDLLExtension));
    r = locale.GetLocaleDllName(ELocaleLanguageSettings, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test.Printf(_L("dllName looking for %s (%s)\n"), dllName.Ptr(), eloclus.Ptr());
    test(dllName.Find(eloclus) != KErrNotFound);
    
    dllName.FillZ();

    r = locale.GetLocaleDllName(ELocaleCollateSetting, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test(dllName.Find(eloclus) != KErrNotFound);
    
    dllName.FillZ();

    r = locale.GetLocaleDllName(ELocaleLocaleSettings, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test(dllName.Find(eloclus) != KErrNotFound);
    
    dllName.FillZ();

    r = locale.GetLocaleDllName(ELocaleTimeDateSettings, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test(dllName.Find(eloclus) != KErrNotFound);
    
    dllName.FillZ();

    r = locale.LoadLocaleAspect(ELocaleLocaleSettings | ELocaleTimeDateSettings, ELOCLGE);
    test_KErrNone(r);

    r = locale.SaveSystemSettings();
    test_KErrNone(r);

    testExtendedUS(ELocaleLanguageSettings | ELocaleCollateSetting, locale);

    symbol.Set();
    test(symbol.Compare(TPtrC(_S("DM"))) == 0);

    TShortDateFormatSpec shortdate;
    shortdate.Set();
    test(shortdate.Compare(TPtrC(_S("%F%*D.%*M.%Y"))) == 0);

    TLongDateFormatSpec longdate;
    longdate.Set();
    test(longdate.Compare(TPtrC(_S("%F%*D%X %N %Y"))) == 0);

    TTimeFormatSpec spec;
    spec.Set();
    test(spec.Compare(TPtrC(_S("%F%H:%T:%S"))) == 0);

    TFileName eloclge;
    eloclge.Copy(ELOCLGE);
    eloclge.Append(KDLLExtension);
    r = locale.GetLocaleDllName(ELocaleLanguageSettings, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test(dllName.Find(eloclus) != KErrNotFound);
    
    dllName.FillZ();

    r = locale.GetLocaleDllName(ELocaleCollateSetting, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test(dllName.Find(eloclus) != KErrNotFound);
    
    dllName.FillZ();

    r = locale.GetLocaleDllName(ELocaleLocaleSettings, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test(dllName.Find(eloclge) != KErrNotFound);
    
    dllName.FillZ();

    r = locale.GetLocaleDllName(ELocaleTimeDateSettings, dllName);
    test_KErrNone(r);
	dllName.UpperCase();
    test(dllName.Find(eloclge) != KErrNotFound);
    
    dllName.FillZ();

    }

void testDigitsInMonthsNames()
    {
    test.Next(_L("Testing date parsing when month names include digits"));

    TTime time;
    _LIT(KMonthJan, "30 Th1 2006 14:31:50");
    test_Equal(KErrArgument, time.Parse(KMonthJan));
    test_KErrNone(UserSvr::ChangeLocale(ELOCLUS2));
    test_Equal(EParseDatePresent | EParseTimePresent, time.Parse(KMonthJan));
    test_KErrNone(UserSvr::ChangeLocale(ELOCL_DEFAULT));
    }

GLDEF_C void CallTestsL(void)
    {

    test.Title();
    test.Start(_L("Starting T_DLOCL tests"));

    testChangeLocale();
    testChangeKeyData();
    testExtendedLocale();
    testDigitsInMonthsNames();
    test.Next(_L("Close connection to the file server\n"));

    test.End();
    }

