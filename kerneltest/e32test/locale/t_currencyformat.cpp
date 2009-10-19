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
// e32test\locale\t_currencyformat.cpp
// Overview:
// Test the currency formatting capabilities of the locale class.
// API Information:
// TLocale.
// Details:
// - Set the currency symbol position before the currency amount.
// - Set no space between the currency symbol and the currency amount.
// - Set 2 decimal places to currency values. 
// - Set negative currency value to be enclosed in brackets without a minus sign.
// - Allow triads in currency values.
// - Set comma as separator to separate groups of three digits to the left of the decimal
// separator.
// - Set dot as separator to separate a whole number from its fractional part.
// - Set currency symbol to pound.
// - Test some simple English Locale currency formats and check it is as expected.
// - Render a currency value as text, based on the locale's currency and numeric format 
// settings and check the Overflow handling capability is as expected.
// - Change a variety of locale settings and verify currency formatting is as expected.
// - Change a variety of locale settings display the results.
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h> 
#include <e32std.h>
#include <e32std_private.h>

#ifdef __VC32__
    // Solve compilation problem caused by non-English locale
    #pragma setlocale("english")
#endif

_LIT(KNewLine, "\n");
LOCAL_D RTest test(_L("T_CurrencyFormat"));
LOCAL_D TLocale locale; 


class TTestOverflowHandler : public TDesOverflow
	{
	void Overflow(TDes& aDes);
	};

void TTestOverflowHandler::Overflow(TDes& aDes)
	{
	test.Printf(_L(" %S is too large... Overflow Handled!!"), &aDes);
	test.Printf(KNewLine);
	}


void SetEnglishCurrency()
	{
	locale.SetCurrencySymbolPosition(ELocaleBefore);
	locale.SetCurrencySpaceBetween(EFalse);
	locale.SetCurrencyDecimalPlaces(2);
	locale.SetCurrencyNegativeInBrackets(ETrue);
	locale.SetCurrencyTriadsAllowed(ETrue);
	locale.SetThousandsSeparator(',');
	locale.SetDecimalSeparator('.');
	User::SetCurrencySymbol(_L("\xA3"));
	}

void TestSimpleEnglishCurrencyFormat()
	{

	TBuf<32> fcurrency; 
	// Set a few of the locale variables and then test that formatting the amount
	// returns what we expected..... 
	test.Start(_L("Testing simple English Locale currency formats"));
	locale.FormatCurrency(fcurrency, 4000);
	test(fcurrency==_L("\xA3\x34\x30.00"));

	locale.FormatCurrency(fcurrency, -567654);
	test(fcurrency==_L("(\xA3\x35,676.54)"));
	
	locale.FormatCurrency(fcurrency,2);
	test(fcurrency==_L("\xA3\x30.02"));

	locale.FormatCurrency(fcurrency,12500000);
	test(fcurrency==_L("\xA3\x31\x32\x35,000.00"));

	locale.FormatCurrency(fcurrency, 00001);
	test(fcurrency==_L("\xA3\x30.01"));
	test.Printf(_L("SimplEnglishCurrencyPass"));
	}

void TestOverflowCapacity()
	{
	// try to test each stage that overflow at occur
	TBuf<10> smallcurrency;
	TTestOverflowHandler handler; 
	test.Next(_L("Testing the Overflow handling capability"));
	// test the overflow handling capability
	
	locale.SetCurrencyDecimalPlaces(11);
	locale.FormatCurrency(smallcurrency,handler,1);
	test(smallcurrency!=_L("0.00000000001"));
	locale.SetCurrencyDecimalPlaces(2);

	locale.FormatCurrency(smallcurrency,handler, -1234567890);
	test(smallcurrency!=_L("(\xA3\x31\x32\x33\x34\x35\x36\x37\x38.90)"));

	locale.FormatCurrency(smallcurrency, handler, 95000000);
	test(smallcurrency!=_L("\xA3\x39\x35\x30,000.00"));

	locale.SetCurrencySpaceBetween(ETrue);
	locale.FormatCurrency(smallcurrency, handler, 2000000);
	test(smallcurrency!=_L("\xA3 20,000.00"));

	locale.FormatCurrency(smallcurrency, handler, -240000);
	test(smallcurrency!=_L("(\xA3 2,400.00)"));

	User::SetCurrencySymbol(_L("Fake\xA3"));
	locale.FormatCurrency(smallcurrency, handler, 240);
	test(smallcurrency==_L("Fake\xA3 2.40"));

	locale.SetCurrencySpaceBetween(EFalse);
	locale.FormatCurrency(smallcurrency, handler, -240);
	test(smallcurrency!=_L("(Fake\xA3\x32.40)"));

	test.Printf(_L("Overflow Pass"));
		
	}

void TestChangeInformation()
	{
	TBuf<32> fcurrency; 
	// Make changes to the locale currency informationa and check formatting
	test.Next(_L("Changing locale information and testing currency Formatting "));
	// Change the format, add space in between
	locale.SetCurrencySpaceBetween(ETrue);
	locale.FormatCurrency(fcurrency,450);
	test(fcurrency==_L("\xA3 4.50"));
	// change the position of the CurrencySymbol 
	locale.SetCurrencySymbolPosition(ELocaleAfter);
	locale.FormatCurrency(fcurrency, 300000);
	test(fcurrency==_L("3,000.00 \xA3"));
	// test the negative format implementation
	locale.FormatCurrency(fcurrency , -3000);
	test(fcurrency!=_L("-30.00 \xA3"));
	test(fcurrency==_L("(30.00 \xA3)"));
	// call the deprecated function to check for compatability
	locale.SetCurrencyNegativeInBrackets(ETrue);
	locale.SetNegativeLoseSpace(ETrue);
	locale.SetNegativeCurrencySymbolOpposite(ETrue);
	locale.FormatCurrency(fcurrency,-2500);
	locale.SetNegativeCurrencySymbolOpposite(EFalse);
	locale.SetNegativeLoseSpace(EFalse);
	test(fcurrency==_L("(\xA3\x32\x35.00)"));

	locale.SetCurrencySymbolPosition(ELocaleBefore);
	locale.FormatCurrency(fcurrency, 300000);
	test(fcurrency==_L("\xA3 3,000.00"));
	// test the negative format implementation
	locale.FormatCurrency(fcurrency , -3000);
	test(fcurrency!=_L("-30.00 \xA3"));
	test(fcurrency==_L("(\xA3 30.00)"));
	// call the deprecated function to check for compatability
	locale.SetCurrencyNegativeInBrackets(ETrue);
	locale.SetNegativeLoseSpace(ETrue);
	locale.SetNegativeCurrencySymbolOpposite(ETrue);
	locale.FormatCurrency(fcurrency,-2500);
	locale.SetNegativeCurrencySymbolOpposite(EFalse);
	test(fcurrency==_L("(25.00\xA3)"));
	// test the New NegativeCurrencyFormat
	locale.SetNegativeCurrencyFormat(TLocale::EInterveningMinusSign);
	locale.FormatCurrency(fcurrency,-32500);
	test(fcurrency==_L("\xA3-325.00"));
	// test the decimal places implementation
	locale.SetCurrencyDecimalPlaces(6);
	locale.FormatCurrency(fcurrency,-32500);
	locale.SetNegativeLoseSpace(EFalse);
	test(fcurrency==_L("\xA3-0.032500"));
	// testing Thousandseparators and triads
	locale.SetCurrencyDecimalPlaces(2);
	locale.SetThousandsSeparator(' ');
	locale.SetCurrencySymbolPosition(ELocaleAfter);
	locale.FormatCurrency(fcurrency, 300000000);
	test(fcurrency==_L("3 000 000.00 \xA3"));

	locale.SetCurrencyTriadsAllowed(EFalse);
	locale.FormatCurrency(fcurrency, 300000000);
	test(fcurrency==_L("3000000.00 \xA3"));
	// test the other NegativeCurrencyFormat Options
	locale.SetNegativeCurrencyFormat(TLocale::ETrailingMinusSign);
	locale.FormatCurrency(fcurrency, -24000000);
	test(fcurrency==_L("240000.00 \xA3-"));

	locale.SetNegativeCurrencyFormat(TLocale::EInterveningMinusSign);
	locale.FormatCurrency(fcurrency, -40000000);
	test(fcurrency== _L("400000.00- \xA3"));
	// change the currency symbol 
	User::SetCurrencySymbol(_L("NZ$"));
	locale.FormatCurrency(fcurrency,250000);
	test(fcurrency!=_L("2,500.00 \xA3"));
	test(fcurrency==_L("2500.00 NZ$"));
	// move the currency symbol forward
	locale.SetCurrencySymbolPosition(ELocaleBefore);
	locale.FormatCurrency(fcurrency,300);
	test(fcurrency==_L("NZ$ 3.00"));

	locale.SetCurrencySpaceBetween(EFalse);
	locale.FormatCurrency(fcurrency, 43020);
	test(fcurrency==_L("NZ$430.20"));

	locale.SetCurrencyTriadsAllowed(ETrue);
	locale.SetThousandsSeparator(',');
	locale.SetCurrencySpaceBetween(ETrue);
	locale.FormatCurrency(fcurrency,-2450000);
	test(fcurrency==_L("NZ$ -24,500.00"));
	locale.SetNegativeCurrencySymbolOpposite(EFalse);
	
	
	}

void TestDisplyCurrencyFormats() 
	{

	// Display Differenct locale currency as a test 
	test.Next(_L("Samples of a few Currency formats that can be displayed"));
	TBuf<32> fcurrency; 
	locale.FormatCurrency (fcurrency, 2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetCurrencySpaceBetween(EFalse);
	User::SetCurrencySymbol(_L("\xA3"));
	locale.SetCurrencySymbolPosition(ELocaleAfter);
	locale.FormatCurrency (fcurrency, 2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetCurrencySymbolPosition(ELocaleBefore);
	User::SetCurrencySymbol(_L("$"));
	locale.SetNegativeCurrencyFormat(TLocale::EInBrackets);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetNegativeCurrencyFormat(TLocale::ELeadingMinusSign);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetNegativeCurrencySymbolOpposite(ETrue);
	locale.SetNegativeLoseSpace(ETrue);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetNegativeCurrencySymbolOpposite(EFalse);
	locale.SetNegativeCurrencyFormat(TLocale::EInterveningMinusSign);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	//******************

	locale.SetCurrencySymbolPosition(ELocaleAfter);
	User::SetCurrencySymbol(_L("$"));
	locale.SetNegativeCurrencyFormat(TLocale::EInBrackets);
	locale.SetNegativeLoseSpace(ETrue);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetNegativeCurrencyFormat(TLocale::ELeadingMinusSign);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetNegativeCurrencySymbolOpposite(ETrue);
	locale.SetNegativeLoseSpace(ETrue);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);
	locale.SetNegativeCurrencySymbolOpposite(EFalse);
	locale.SetNegativeCurrencyFormat(TLocale::EInterveningMinusSign);
	locale.FormatCurrency (fcurrency, -2500000);
	test.Printf(fcurrency);
	test.Printf(KNewLine);

	//******************
	TInt64 x=25000000;
	x*=10000;
	locale.FormatCurrency (fcurrency,x );
	test.Printf(fcurrency);
	test.Printf(KNewLine);


	}

GLDEF_C TInt E32Main(void)
	{

	// Call all the tests from here 
	test.Title();
	SetEnglishCurrency(); 
	TestSimpleEnglishCurrencyFormat();
	TestOverflowCapacity();
	SetEnglishCurrency(); 
	TestChangeInformation();
	TestDisplyCurrencyFormats();
	test.End();
	return KErrNone; 
	}
