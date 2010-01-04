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
* Default locale settings for Scandinavian countries (English)
*
*/


#include "ls_std.h"
#include "complocl.h"

// The configuration data
const TLanguage LLocaleData::Language = ELangEnglish;
const TInt LLocaleData::CountryCode = 44;
const TInt LLocaleData::UniversalTimeOffset = 0;
const TDateFormat LLocaleData::DateFormat = EDateEuropean;
const TTimeFormat LLocaleData::TimeFormat = ETime24;
const TLocalePos LLocaleData::CurrencySymbolPosition = ELocaleAfter;
const TBool LLocaleData::CurrencySpaceBetween = ETrue;
const TInt LLocaleData::CurrencyDecimalPlaces = 2;
const TLocale::TNegativeCurrencyFormat LLocaleData::NegativeCurrencyFormat=TLocale::TNegativeCurrencyFormat(0); // replacing CurrencyNegativeInBrackets
const TBool LLocaleData::CurrencyTriadsAllowed = ETrue;
const TText * const LLocaleData::ThousandsSeparator = _S(".");
const TText * const LLocaleData::DecimalSeparator = _S(",");
const TText * const LLocaleData::DateSeparator[KMaxDateSeparators] = {_S(""),_S("-"),_S("-"),_S("")};
const TText * const LLocaleData::TimeSeparator[KMaxTimeSeparators] = {_S(""),_S(":"),_S(":"),_S("")};
const TLocalePos LLocaleData::AmPmSymbolPosition = ELocaleAfter;
const TBool LLocaleData::AmPmSpaceBetween = ETrue;
//const TUint LLocaleData::DaylightSaving = EDstNone;
const TDaylightSavingZone LLocaleData::HomeDaylightSavingZone = EDstEuropean;
const TUint LLocaleData::WorkDays = 0x1f;
const TText * const LLocaleData::CurrencySymbol = _S("kr");
const TText* const LLocaleData::ShortDateFormatSpec = _S("%F%*D-%*M-%Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LLocaleData::LongDateFormatSpec = _S("%F%*D%X %N %Y"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TText* const LLocaleData::TimeFormatSpec = _S("%F%H:%T:%S"); // needs checking by a localisation team (this item was added since real localisation - the value given here has been set by a software developer so it may be wrong)
const TFatUtilityFunctions* const LLocaleData::FatUtilityFunctions = NULL;
const TDay LLocaleData::StartOfWeek = EMonday;
const TClockFormat LLocaleData::ClockFormat = EClockDigital;
const TUnitsFormat LLocaleData::UnitsGeneral = EUnitsMetric;
const TUnitsFormat LLocaleData::UnitsDistanceShort = EUnitsMetric;
const TUnitsFormat LLocaleData::UnitsDistanceLong = EUnitsMetric;
const TUint LLocaleData::ExtraNegativeCurrencyFormatFlags = 0;
const TLanguage LLocaleData::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LLocaleData::DigitType = EDigitTypeWestern;

// end of LS_CYSC.CPP
