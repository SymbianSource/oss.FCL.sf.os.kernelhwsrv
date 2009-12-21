/*
* Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


#include <ls_std.h>
#include "complocl.h"


/**
Gets the current language type.
@return language type.
*/
EXPORT_C TLanguage Locl::Language()
	{

	return(LLocaleData::Language);
	}

/**
Creates the localisation table.
@param aLocale A pointer to the structure to be created. It holds 
the system's locale settings.
*/
EXPORT_C void Locl::LocaleData(SLocaleData *aLocale)
	{

	aLocale->iCountryCode=LLocaleData::CountryCode;
	aLocale->iUniversalTimeOffset=LLocaleData::UniversalTimeOffset;
	aLocale->iDateFormat=LLocaleData::DateFormat;
	aLocale->iTimeFormat=LLocaleData::TimeFormat;
	aLocale->iCurrencySymbolPosition=LLocaleData::CurrencySymbolPosition;
	aLocale->iCurrencySpaceBetween=LLocaleData::CurrencySpaceBetween;
	aLocale->iCurrencyDecimalPlaces=LLocaleData::CurrencyDecimalPlaces;
	aLocale->iNegativeCurrencyFormat=TNegativeCurrencyFormat(LLocaleData::NegativeCurrencyFormat); // replaces iCurrencyNegativeInBrackets
	aLocale->iCurrencyTriadsAllowed=LLocaleData::CurrencyTriadsAllowed;
	aLocale->iThousandsSeparator=*LLocaleData::ThousandsSeparator;
	aLocale->iDecimalSeparator=*LLocaleData::DecimalSeparator;
	TInt i=0;
	for(;i<KMaxDateSeparators;i++)
		aLocale->iDateSeparator[i]=*LLocaleData::DateSeparator[i];
	for(i=0;i<KMaxTimeSeparators;i++)
		aLocale->iTimeSeparator[i]=*LLocaleData::TimeSeparator[i];
	aLocale->iAmPmSymbolPosition=LLocaleData::AmPmSymbolPosition;
	aLocale->iAmPmSpaceBetween=LLocaleData::AmPmSpaceBetween;
//	aLocale->iDaylightSaving=LLocaleData::DaylightSaving;
	aLocale->iHomeDaylightSavingZone=LLocaleData::HomeDaylightSavingZone;
	aLocale->iWorkDays=LLocaleData::WorkDays;
	aLocale->iStartOfWeek=LLocaleData::StartOfWeek;
	aLocale->iClockFormat=LLocaleData::ClockFormat;
	aLocale->iUnitsGeneral=LLocaleData::UnitsGeneral;
	aLocale->iUnitsDistanceLong=LLocaleData::UnitsGeneral;
	aLocale->iUnitsDistanceShort=LLocaleData::UnitsGeneral;
	aLocale->iExtraNegativeCurrencyFormatFlags=LLocaleData::ExtraNegativeCurrencyFormatFlags;
	aLocale->iLanguageDowngrade[0] = static_cast<TUint16>(LLocaleData::LanguageDowngrade[0]);
	aLocale->iLanguageDowngrade[1] = static_cast<TUint16>(LLocaleData::LanguageDowngrade[1]);
	aLocale->iLanguageDowngrade[2] = static_cast<TUint16>(LLocaleData::LanguageDowngrade[2]);
	}

/**
Gets the address of the currency symbol, e.g. '$' for US dollar.
@return The address of the currency symbol.
*/
EXPORT_C const TText * Locl::CurrencySymbol()
	{

	return(LLocaleData::CurrencySymbol);
	}

/**
Gets the address of the short date format.
@return The address of the short date format.
*/
EXPORT_C const TText* Locl::ShortDateFormatSpec()
	{

	return(LLocaleData::ShortDateFormatSpec);
	}

/**
Gets the address of the long date format.
@return The address of the long date format.
*/
EXPORT_C const TText* Locl::LongDateFormatSpec()
	{

	return(LLocaleData::LongDateFormatSpec);
	}

/**
Gets the address of the time format.
@return The address of the time format.
*/
EXPORT_C const TText* Locl::TimeFormatSpec()
	{

	return(LLocaleData::TimeFormatSpec);
	}

/**
Gets the address of the FAT utility functions.
@return The address of the FAT utility functions.
*/
EXPORT_C const TFatUtilityFunctions* Locl::FatUtilityFunctions()
	{

	return(LLocaleData::FatUtilityFunctions);
	}

/**
Gets the address of the date suffix table. A date suffix table
stores the suffix strings of the 31 days in a month, e.g. in English, 
"st" for first day of the month, "nd" for the second day of the month.
@return The address of the date suffix table.
*/
EXPORT_C const TText * const * Locl::DateSuffixTable()
	{

	return(&LLanguage::DateSuffixTable[0]);
	}

/**
Gets the address of the day table, which stores the names 
of weekdays. In English, it starts with "Monday" and ends
with "Sunday".
@return The address of the day table.
*/
EXPORT_C const TText * const * Locl::DayTable()
	{

	return(&LLanguage::DayTable[0]);
	}

/**
Gets the address of the abbreviated day table, which stores 
the abbreviated names of weekdays. In English, it starts
with "Mon" and ends with "Sun".
@return The address of the abbreviated day table.
*/
EXPORT_C const TText * const * Locl::DayAbbTable()
	{

	return(&LLanguage::DayAbbTable[0]);
	}

/**
Gets the address of the month table, which stores 
the names of the months. In English, it starts with 
"January", and ends with "December".
@return The address of the month table.
*/
EXPORT_C const TText * const * Locl::MonthTable()
	{

	return(&LLanguage::MonthTable[0]);
	}

/**
Gets the address of the abbreviated month table, which stores 
the abbreviated names of the months, In English, it starts
with "Jan", and ends with "Dec".
@return The address of the month table.
*/
EXPORT_C const TText * const * Locl::MonthAbbTable()
	{

	return(&LLanguage::MonthAbbTable[0]);
	}

/**
Gets the address of the AmPm table, which stores the expression 
for the morning and the afternoon, in English, "am" for the
morning and "pm" for the afternoon.
@return The address of the AmPm table.
*/
EXPORT_C const TText * const * Locl::AmPmTable()
	{

	return(&LLanguage::AmPmTable[0]);
	}

/**
Gets the address of the message table. The message table contains 
messages that the base software may need to issue without the 
benefit of access to compiled resources.
@return The address of the message table.
*/
EXPORT_C const TText * const * Locl::MsgTable()
	{

	return(&LMessages::MsgTable[0]);
	}

/**
Gets the address of the locale character set object which contains 
collation rules etc. It is used in Unicode builds to supply 
locale-specific character attribute and collation data.
@return The address of the locale character set object, or NULL 
in case of a non-UNICODE build.
*/
EXPORT_C const LCharSet* Locl::CharSet()
	{
	#ifdef _UNICODE
		return &TheCharSet;
	#else
		return NULL;
	#endif
	}

/**
Gets the address of the character type conversion table.
The character type conversion table does not exist in 
the Unicode build. This table has 256 items which classifies
256 ASCII codes into: Uppercase letter, Lowercase letter, 
Punctuation, Decimal digit etc..
@return The address of the character type conversion table, 
or NULL in case of a UNICODE build.
*/
EXPORT_C const TUint8 * Locl::TypeTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::TypeTable[0]);
	#endif 		
	}


/**
Gets the address of the uppercase table. The uppercase table 
does not exist in the Unicode build. It is used to convert 
the letter in lowercase to uppercase.
@return The address of the uppercase table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * Locl::UpperTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::UpperTable[0]);
	#endif 

	}

/**
Gets the address of the lowercase table. The lowercase table
does not exist in the Unicode build. It is used to convert 
the letter in uppercase to lowercase.
@return The address of the lowercase table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * Locl::LowerTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::LowerTable[0]);
	#endif 
	}

/**
Gets the address of the fold table. The fold table does not exist 
in the Unicode build. It is used to fold the character according
to a specified folding method: converting characters to their 
lower case form, if any; stripping accents; converting digits 
representing values 0..9 to characters '0'..'9' etc..
@return The address of the fold table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * Locl::FoldTable()
	{
	#ifdef _UNICODE
		return NULL;

	#else
		return(&LAlphabet::FoldTable[0]);
	#endif 
	}

/**
Gets the address of the collate table. The collate table does
not exist in the Unicode build. This table is used to collate
strings to remove differences between characters that are deemed 
unimportant for the purposes of ordering characters.
@return The address of the collate table, or NULL
in case of a UNICODE build.
*/
EXPORT_C const TText * Locl::CollTable()
	{
	#ifdef _UNICODE
		return NULL;
	#else
		return(&LAlphabet::CollTable[0]);
	#endif 
	}

/**
Check whether it is a Unicode Build.
@return ETrue for Unicode Build, EFalse for non-Unicode Build.
*/
EXPORT_C TBool Locl::UniCode()
	{
	#ifdef _UNICODE
		return ETrue;
	#else
		return EFalse;
	#endif 
	}
