// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\locl\t_table.cpp
// 
//

#include <kernel/localise.h>

EXPORT_C TLanguage Locl::Language()
//
// Returns the language type.
//
	{

	return(LCountry::Language);
	}

EXPORT_C void Locl::LocaleData(SLocaleData *aLocale)
//
// Create the localisation table.
//
	{

	aLocale->iCountryCode=LCountry::CountryCode;
	aLocale->iUniversalTimeOffset=LCountry::UniversalTimeOffset;
	aLocale->iDateFormat=LCountry::DateFormat;
	aLocale->iTimeFormat=LCountry::TimeFormat;
	aLocale->iCurrencySymbolPosition=LCountry::CurrencySymbolPosition;
	aLocale->iCurrencySpaceBetween=LCountry::CurrencySpaceBetween;
	aLocale->iCurrencyDecimalPlaces=LCountry::CurrencyDecimalPlaces;
	aLocale->iNegativeCurrencyFormat=(TNegativeCurrencyFormat)LCountry::CurrencyNegativeInBrackets;
	aLocale->iCurrencyTriadsAllowed=LCountry::CurrencyTriadsAllowed;
	aLocale->iThousandsSeparator=*LCountry::ThousandsSeparator;
	aLocale->iDecimalSeparator=*LCountry::DecimalSeparator;
	TInt i=0;
	for(;i<KMaxDateSeparators;i++)
		aLocale->iDateSeparator[i]=*LCountry::DateSeparator[i];
	for(i=0;i<KMaxTimeSeparators;i++)
		aLocale->iTimeSeparator[i]=*LCountry::TimeSeparator[i];
	aLocale->iAmPmSymbolPosition=LCountry::AmPmSymbolPosition;
	aLocale->iAmPmSpaceBetween=LCountry::AmPmSpaceBetween;
//	aLocale->iDaylightSaving=LCountry::DaylightSaving;
	aLocale->iHomeDaylightSavingZone=LCountry::HomeDaylightSavingZone;
	aLocale->iWorkDays=LCountry::WorkDays;
	aLocale->iStartOfWeek=LCountry::StartOfWeek;
	aLocale->iClockFormat=LCountry::ClockFormat;
	aLocale->iUnitsGeneral=LCountry::UnitsGeneral;
	aLocale->iUnitsDistanceLong=LCountry::UnitsGeneral;
	aLocale->iUnitsDistanceShort=LCountry::UnitsGeneral;
	}

EXPORT_C const TText * Locl::CurrencySymbol()
//
// Returns the address of the currency symbol.
//
	{

	return(LCountry::CurrencySymbol);
	}

EXPORT_C const TText* Locl::ShortDateFormatSpec()
//
// Returns the address of the short date format.
//
	{

	return(LCountry::ShortDateFormatSpec);
	}

EXPORT_C const TText* Locl::LongDateFormatSpec()
//
// Returns the address of the long date format.
//
	{

	return(LCountry::LongDateFormatSpec);
	}

EXPORT_C const TText* Locl::TimeFormatSpec()
//
// Returns the address of the long date format.
//
	{

	return(LCountry::TimeFormatSpec);
	}

EXPORT_C const TFatUtilityFunctions* Locl::FatUtilityFunctions()
//
// Returns the addresses of the FAT utility functions.
//
	{

	return(LCountry::FatUtilityFunctions);
	}

EXPORT_C const TText * const * Locl::DateSuffixTable()
//
// Returns the address of the date suffix table.
//
	{

	return(&LLanguage::DateSuffixTable[0]);
	}

EXPORT_C const TText * const * Locl::DayTable()
//
// Returns the address of the day table.
//
	{

	return(&LLanguage::DayTable[0]);
	}

EXPORT_C const TText * const * Locl::DayAbbTable()
//
// Returns the address of the abbreviated day table.
//
	{

	return(&LLanguage::DayAbbTable[0]);
	}

EXPORT_C const TText * const * Locl::MonthTable()
//
// Returns the address of the month table.
//
	{

	return(&LLanguage::MonthTable[0]);
	}

EXPORT_C const TText * const * Locl::MonthAbbTable()
//
// Returns the address of the abbreviated month table.
//
	{

	return(&LLanguage::MonthAbbTable[0]);
	}

EXPORT_C const TText * const * Locl::AmPmTable()
//
// Returns the address of the AmPm table.
//
	{

	return(&LLanguage::AmPmTable[0]);
	}

EXPORT_C const TText * const * Locl::MsgTable()
//
// Returns the address of the message table.
//
	{

	return(&LMessages::MsgTable[0]);
	}

// Return the address of the locale character set object: contains collation rules etc.
EXPORT_C const LCharSet* Locl::CharSet()
	{
	#ifdef _UNICODE
		return &TheCharSet;
	#else
		return NULL;
	#endif
	}

// The functions returning locale-specific character attribute tables do not exist in the Unicode build.
EXPORT_C const TUint8 * Locl::TypeTable()
//
// Return the address of the type table.
//
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::TypeTable[0]);
	#endif 		
	}


EXPORT_C const TText * Locl::UpperTable()
//
// Return the address of the upper table.
//
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::UpperTable[0]);
	#endif 

	}


EXPORT_C const TText * Locl::LowerTable()
//
// Return the address of the lower table.
//
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::LowerTable[0]);
	#endif 
	}


EXPORT_C const TText * Locl::FoldTable()
//
// Return the address of the fold table.
//
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::FoldTable[0]);
	#endif 
	}

EXPORT_C const TText * Locl::CollTable()
//
// Return the address of the collate table.
//
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::CollTable[0]);
	#endif 
	}

EXPORT_C TBool Locl::UniCode()
//
// Returns ETrue for Unicode Build 
//
	{
	#ifdef _UNICODE
		return ETrue;
	#else
		return EFalse;
	#endif 
	}

